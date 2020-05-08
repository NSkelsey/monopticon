// Like tcpdump but with music.

#include <csound/csound.hpp>
#include <csound/csound_threaded.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <chrono>

#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <sys/inotify.h>
#include <limits.h>

#include <event.h>
#include <sys/time.h>

#include <Corrade/Utility/Arguments.h>
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

struct instrument_map {
  std::string g_c_f;
  std::string g_e_f;
  std::string g_chord_dir;

  bool t_active;
};

struct inot_params {
  int ifd;
  instrument_map i_map;
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


// monitor-dir uses inotify to start monitoring a directory for changes to
// files inside. Returns a file descriptor of the notifier to the caller.
inot_params parse_arguments(int argc, char *argv[]) {
  Corrade::Utility::Arguments args;
  args.addBooleanOption('v', "verbose").setHelp("verbose", "log verbosely")
      .setGlobalHelp("Read stdin to play background; Watches EVENT-DIR to play other instruments")
      .addBooleanOption('N', "no-stdin").setHelp("no-stdin", "Disables read from stdin.")
      .addOption("g-c-file", "ping.log").setHelp("g-c-file", "C major")
      .addOption("g-e-file", "pong.log").setHelp("g-e-file", "E")
      .addArgument("event-dir").setHelp("event-dir", "The directory that contains event log files.")
      .parse(argc, argv);

  inot_params inot_p = {
    .i_map = {
      .g_c_f = args.value<std::string>("g-c-file"),
      .g_e_f = args.value<std::string>("g-e-file"),
      .g_chord_dir = args.value<std::string>("event-dir"),
      .t_active = !args.isSet("no-stdin")
    }
  };

  /* Create inotify instance */
  int inotifyFd = inotify_init();
  if (inotifyFd == -1) {
    fprintf(stderr, "inotify_init failed unexpectedly\n");
    exit(1);
  }
  inot_p.ifd = inotifyFd;

  std::string dir = args.value<std::string>("event-dir");
  int wd = inotify_add_watch(inotifyFd, dir.c_str(), IN_MODIFY);
  if (wd == -1) {
    fprintf(stderr, "inotify_add_watch failed unexpectedly\n");
    exit(2);
  }

  return inot_p;
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
void handle_stats_event(int fd, short evnt, void *z) {
  static const long max_len = 30;

  char *line = NULL;
  size_t size = 0;
  int nread = getline(&line, &size, stdin); // TODO handle error
  if (nread == 0) {
    fprintf(stderr, "Failed to read line from stdin");
    return;
  }

  int num_pkts = atoi(line);
  mov_avg = compute_exp_mov_avg(mov_avg, num_pkts);

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



// read_from_inotify is called whenever z->ifd becomes readable. This function
// handles the decision to add a note to the current bar_beat.
void read_from_inotify(int fd, short evnt, void *z) {
  inot_params *v = (inot_params *)z;
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

    if (strcmp(event->name, v->i_map.g_c_f.c_str()) == 0) {
      play_guitar(&bar_b, 1);
    } else if (strcmp(event->name, v->i_map.g_e_f.c_str()) == 0) {
      play_guitar(&bar_b, 2);
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
  inot_params inot_p = parse_arguments(argc, argv);

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

  struct timeval tv2;
  struct event ev2 = *event_new(
    EB,
    inot_p.ifd,
    EV_READ|EV_PERSIST,
    read_from_inotify,
    &inot_p
  );
  event_add(&ev2, &tv2);


  if (inot_p.i_map.t_active) {
    struct timeval tv3;
    struct event ev3 = *event_new(
      EB,
      0, // STDIN
      EV_READ|EV_PERSIST,
      handle_stats_event,
      NULL
    );
    event_add(&ev3, &tv3);
  }

  event_dispatch();
}