// Like tcpdump but with music.
// capstats -I 1 -i wlp3s0 2>&1 | tee listen/stats.txt
// cd listen
// /opt/zeek/bin/zeek -i wlp3s0 -b ../sound-filters.zeek

#include <csound/csound.hpp>
#include <csound/csound_threaded.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <limits.h>

#include <event.h>
#include <sys/time.h>

#include <Corrade/Utility/Debug.h>
#include <Corrade/Utility/DebugStl.h>
#include <Corrade/Utility/Resource.h>

const int tendo_pf_size = 5;
const int guitar_pf_size = 3;

// Stores notes to play during next beat
struct bar_beats {
  bool tendo_play;
  bool guitar_play;
  double tendo_pfield[tendo_pf_size];  // maps to csound parameters
  double guitar_pfield[guitar_pf_size]; // idem
};

event_base *EB;
Csound *csound;
void * threadID;

bool in_shutdown = false;

void sig_handler(int signo) {
  if(signo == SIGINT && !in_shutdown) {
    in_shutdown = true;
    printf("Entered shutdown from SIGINT\n");
    event_base_loopbreak(EB);

    double pfield[tendo_pf_size] = {9, 0, 0.5, 1, 2};

    csound->ScoreEvent('i', pfield, tendo_pf_size);
    usleep(500000);

    csound->Stop();
    csoundJoinThread(threadID);
    exit(9);
  }
}


void usage(int e) {
  fprintf(stderr, "Usage: listen <log-dir>\n");
  exit(e);
}


// parse_first_arg uses inotify to start monitoring a directory for changes to
// files inside. Returns a file descriptor of the notifier to the caller.
int parse_first_arg(int argc, char *argv[]) {
  if (argc != 2) {
    usage(1);
  }

  /* Create inotify instance */
  int inotifyFd = inotify_init();
  if (inotifyFd == -1) {
    fprintf(stderr, "inotify_init failed\n");
    usage(2);
  }

  int wd = inotify_add_watch(inotifyFd, argv[1], IN_MODIFY);
  if (wd == -1) {
    printf("inotify_add_watch failed\n");
    usage(3);
  }

  return inotifyFd;
}


FILE* open_stats_file(int argc, char *argv[]) {
  char s[255];
  // TODO deal with sprintf overflow.
  sprintf(s, "%s/stats.txt", argv[1]);

  FILE *fd = fopen(s, "r");
  if (fd == nullptr) {
    fprintf(stderr, "Requires a file in %s called stats.txt\n", argv[2]);
    usage(4);
  }

  return fd;
}


// configureCsound sets up and starts a threaded instance of Csound.
void configureCsound() {
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

  std::stringstream buffer;
  Corrade::Utility::Resource rs{"sounds"};

  buffer << rs.get("clean/orchestra.csd");

  res = csound->CompileCsdText(buffer.str().c_str());
  assert(res == CSOUND_SUCCESS);

  res = csound->Start();
  assert(res == CSOUND_SUCCESS);
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
  CSOUND *c = csound->GetCsound();

  if (bar_b.guitar_play) {
    csoundScoreEventAsync(c, 'i', bar_b.guitar_pfield, guitar_pf_size);
  }

  bool sustain_tendo = false;
  if (bar_b.tendo_play) {
    csoundScoreEventAsync(c, 'i', bar_b.tendo_pfield, tendo_pf_size);
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

  double lforates[3] = {1.0, 4.0, 8.0};
  double pfield[tendo_pf_size] = {9, 0, 0.5, 3, lforates[level -1]};

  memcpy(bar_b->tendo_pfield, &pfield, tendo_pf_size * sizeof(double));
}


void play_guitar(bar_beats *bar_b, int instrument) {
  bar_b->guitar_play = true;

  double pfield[guitar_pf_size] = {(double)instrument, 0, 6};

  memcpy(bar_b->guitar_pfield, &pfield, guitar_pf_size * sizeof(double));
}


double mov_avg = 0;

// compute_exp_mov_avg uses a closed form solution for decaying measurements
// over a configurable time span. The algorithm is described on wikipedia:
// http://en.wikipedia.org/wiki/Moving_average#Application_to_measuring_computer_performance
double compute_exp_mov_avg(double last_l, int new_p) {
  double q_n = (double) new_p;

  double l_n = q_n + exp(-0.7) * (last_l - q_n);
  return l_n;
}


// handle_stats_event reads the last line from streamfd and uses the first int
// encountered to update the moving weighted average.
void handle_stats_event(FILE* streamfd) {
/* The line format from capstats follows. We want pkts:
1588776391.095325 pkts=15 kpps=0.0 kbytes=1 mbps=0.0 nic_pkts=169 nic_drops=0 u=0 t=15 i=0 o=0 nonip=0
*/
  static const long max_len = 200;
  char buf[max_len+1];

  fseek(streamfd, -max_len, SEEK_END);
  ssize_t len = fread(buf, 1, max_len, streamfd);

  buf[len] = '\0';
  char *last_newline = strchr(buf, '\n');
  char *last_line = last_newline+1;
  char *last_pkts_val_strt;
  double ts = strtod(last_line, &last_pkts_val_strt);

  int num_pkts = atoi(last_pkts_val_strt+6);
  printf("num_pkts=%d\n", num_pkts);

  mov_avg = compute_exp_mov_avg(mov_avg, num_pkts);
  printf("moving avg: %f\n", mov_avg);

  int level = 1;
  bool play = true;
  if (mov_avg > 5.0 && mov_avg < 15.0) {
    level = 1;
  } else if (mov_avg > 15.0 && mov_avg < 80.0) {
    level = 2;
  } else if (mov_avg > 80.0) {
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


uintptr_t perform(void *p) {
    ((Csound *)p)->Perform();
    return 0;
}

int main(int argc, char *argv[]) {
  int inotifyFd = parse_first_arg(argc, argv);
  FILE* intstreamFD = open_stats_file(argc, argv);

  csoundInitialize(CSOUNDINIT_NO_SIGNAL_HANDLER);
  assert(signal(SIGINT, sig_handler) != SIG_ERR);

  csound = new Csound();
  configureCsound();

  threadID = csoundCreateThread(perform, csound);

  // Setup libevent to handling scheduling
  EB = event_init();

  struct timeval tv;

  tv.tv_sec = 0;
  tv.tv_usec = 500000;

  struct event ev = *event_new(
    EB,
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

  struct timeval tv2;
  struct event ev2 = *event_new(
    EB,
    inotifyFd,
    EV_READ|EV_PERSIST,
    read_from_inotify,
    &notif_params
  );
  event_add(&ev2, &tv2);

  event_dispatch();
}
