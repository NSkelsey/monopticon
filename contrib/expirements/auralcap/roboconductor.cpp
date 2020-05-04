/**
 * Like tcpdump but with music
 * g++ -Wno-write-strings -O2 roboconductor.cpp -oroboconductor -lcsound64 -lpthread -lm
 **/

#include <csound/csound_threaded.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

#include <stdlib.h>
#include <sys/inotify.h>
#include <limits.h>

#include <event.h>
#include <sys/time.h>


// Stores notes to play during next beat
const int tendo_pf_size = 5;
const int guitar_pf_size = 5;
struct bar_beats {
  bool tendo_play;
  bool guitar_play;
  double tendo_pfield[tendo_pf_size];  // maps to csound parameters
  double guitar_pfield[guitar_pf_size]; // idem
};


void usage(int e) {
  fprintf(stderr, "Usage: listen <orchestra.csd> <log-dir>\n");
  exit(e);
}

// parse_first_arg uses inotify to start monitoring a directory for changes to
// files inside. Returns a file descriptor of the notifier to the caller.
int parse_first_arg(int argc, char *argv[]) {
  if (argc != 3) {
    usage(1);
  }

  /* Create inotify instance */
  int inotifyFd = inotify_init();
  if (inotifyFd == -1) {
    fprintf(stderr, "inotify_init failed\n");
    usage(2);
  }

  int wd = inotify_add_watch(inotifyFd, argv[2], IN_MODIFY);
  if (wd == -1) {
    printf("inotify_add_watch failed\n");
    usage(3);
  }

  return inotifyFd;
}


FILE* parse_next_arg(int argc, char *argv[]) {
  char s[255];
  // TODO deal with sprintf overflow.
  sprintf(s, "%s/stats.txt", argv[2]);
  printf(s);

  FILE *fd = fopen(s, "r");
  if (fd == nullptr) {
    fprintf(stderr, "Requires a file in %s called stats.txt\n", argv[2]);
    usage(4);
  }

  return fd;
}


// configureCsound sets up and starts a threaded instance of Csound.
void configureCsound(CsoundThreaded *csound, char c[]) {
  int res;
  res = csound->SetOption("-d");
  if (res != CSOUND_SUCCESS) {
    fprintf(stderr, "opt 1: %d", res);
    exit(2);
  }
  csound->SetOption("-odac");
  if (res != CSOUND_SUCCESS) {
    fprintf(stderr, "opt 2: %d", res);
    exit(3);
  }

  std::ifstream t;
  t.open(c);
  std::stringstream buffer;
  buffer << t.rdbuf();

  csound->CompileCsdText(buffer.str().c_str());

  csound->Start();
  int thread = csound->Perform();
  std::cout << "Performing in thread 0x" << std::hex << thread << "..." << std::endl;
}

void reset_bb(bar_beats *bb) {
  bb->tendo_play = false;
  bb->guitar_play = false;
}


int b = 0;
bar_beats bar_b = {false, false};

// beat_event is called periodically. The fucntions uses csound and bar_beats
// to pass commands to play instruments in the (unmanaged) csound thread.
void beat_event(int fd, short event, void *csptr) {
  CsoundThreaded *csound = (CsoundThreaded *) csptr;

  if (bar_b.guitar_play) {
    csound->ScoreEvent('i', bar_b.guitar_pfield, guitar_pf_size);
  }

  bool sustain_tendo = false;
  if (bar_b.tendo_play) {
    csound->ScoreEvent('i', bar_b.tendo_pfield, tendo_pf_size);
    sustain_tendo = true;
  }

  reset_bb(&bar_b);

  // Maintain tendo play and tempo until new weighted avg decision
  if (sustain_tendo) {
    bar_b.tendo_play = true;
  }

  b++;
}


void play_tendo(bar_beats *bar_b, int level) {
  bar_b->tendo_play = true;

  double lforates[3] = {0.5, 4.0, 8.0};
  double pfield[tendo_pf_size] = {9, 0, 0.5, 3, lforates[level -1]};

  memcpy(bar_b->tendo_pfield, &pfield, tendo_pf_size * sizeof(double));
}


void play_guitar(bar_beats *bar_b, int instrument) {
  bar_b->guitar_play = true;

  double pfield[guitar_pf_size] = {(double)instrument, 0, 6};

  memcpy(bar_b->guitar_pfield, &pfield, guitar_pf_size * sizeof(double));
}

// TODO convert to struct
float weighted_m_avg = 0;
int total = 0;
int numerator = 0;
int samples = 0;

const int len = 5;
int old_p[len] = {0};

float compute_new_avg(int new_p) {
  numerator = numerator + samples*new_p - total;

  total = total + new_p - old_p[abs((samples - len) % len)];
  printf("t=%d, num=%d, samp=%d\n", total, numerator, samples);

  float fsamp = (float)samples;
  float fnum = (float)numerator;

  float new_weighted = fnum / (fsamp*(fsamp+1)/2);
  weighted_m_avg = new_weighted;

  return new_weighted;
}

// TODO should be an event
void handle_stats_event(FILE* streamfd) {
  static const long max_len = 7;
  char buf[max_len + 1];

  fseek(streamfd, -max_len, SEEK_END);
  ssize_t len = fread(buf, 1, max_len, streamfd);

  buf[len] = '\0';
  char *last_newline = strrchr(buf, '\n');
  char *last_line = last_newline+1;

  int num_pkts = atoi(buf);
  printf("num_pkts=%d\n", num_pkts);

  old_p[samples % len] = num_pkts;

  samples += 1;
  compute_new_avg(num_pkts);
  printf("moving avg: %f\n", weighted_m_avg);
 
  int level = 1;
  bool play = true;
  if (weighted_m_avg > 0.0 && weighted_m_avg < 15.0) {
    level = 1;
  } else if (weighted_m_avg > 15.0 && weighted_m_avg < 80.0) {
    level = 2;
  } else if (weighted_m_avg > 80.0) {
    level = 3;
  } else {
    play = false;
  }

  if (play) {
    play_tendo(&bar_b, level);
  } else {
    reset_bb(&bar_b);
  }
}


struct read_inot {
  int ifd;
  FILE* streamfd;
};

// read_from_inotify is called whenever z->ifd becomes readable. This function
// handles the decision to add a note to the current bar_beat.
void read_from_inotify(int fd, short evnt, void *z) {
  read_inot *v = (read_inot *)z;
  int numRead;
  char *p;
  struct inotify_event *event;

  char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));

  numRead = read(v->ifd, buf, sizeof(buf));
  printf("5\n");
  if (numRead == 0) {
    printf("read() from inotify fd returned 0!\n");
    exit(6);
  }

  if (numRead == -1) {
    printf("read() failed\n");
    exit(7);
  }

  for (p = buf; p < buf + numRead; ) {
    event = (struct inotify_event *) p;
    printf("name = %s\n", event->name);

    if (strcmp(event->name, "ping.log") == 0) {
      play_guitar(&bar_b, 1);
    } else if (strcmp(event->name, "pong.log") == 0) {
      play_guitar(&bar_b, 2);
    } else if (strcmp(event->name, "stats.txt") == 0) {
      handle_stats_event(v->streamfd);
    } else {
      int r = rand() % 2 + 2; // play a G or a D, or a lowC
      play_guitar(&bar_b, r);
    }

    p += sizeof(struct inotify_event) + event->len;
  }
}


int main(int argc, char *argv[])
{
  int inotifyFd = parse_first_arg(argc, argv);
  FILE* intstreamFD = parse_next_arg(argc, argv);

  CsoundThreaded *csound = new CsoundThreaded();
  configureCsound(csound, argv[1]);

  // Setup libevent to handling scheduling
  event_base *eb = event_init();

  struct event ev;
  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  ev = *event_new(
    eb,
    -1,
    EV_PERSIST,
    beat_event,
    csound
  );
  event_add(&ev, &tv);


  read_inot notif_params = {
    ifd: inotifyFd,
    streamfd: intstreamFD
  };

  struct event ev2;
  struct timeval tv2;

  ev2 = *event_new(
    eb,
    inotifyFd,
    EV_READ|EV_PERSIST,
    read_from_inotify,
    &notif_params
  );

  event_add(&ev2, &tv2);

  // TODO add third event for specific immediate read from end of the integer stream.

  event_dispatch();
}