#include <csound/csound_threaded.hpp>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <fstream>
#include <chrono>

#include <stdlib.h>
#include <sys/inotify.h>
#include <limits.h>

double pcubefields[][7] = {
  {3, 0, 15, 0, 7.06, 2.0, 0.2},
  {3, 0, 15, 0, 8.01, 2.0, 0.2},
  {3, 0, 15, 0, 8.06, 2.0, 0.2},
  {3, 0, 15, 0, 8.10, 2.0, 0.2},
  {3, 0, 15, 0, 8.11, 2.0, 0.2},
  {3, 0, 15, 0, 9.04, 2.0, 0.2},
};


const int len = 5;
int old_p[len] = {0};

float weighted_m_avg = 0;
int total = 0;
int numerator = 0;
int samples = 0;

int level = 0;
double lforates[4] = {1.0, 2.0, 4.0, 8.0};

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

typedef std::chrono::duration<int,std::milli> millisec_type;
millisec_type one_second (1000);
millisec_type two_second (1950);

#define BUF_LEN (10 * (sizeof(struct inotify_event) + NAME_MAX + 1))

/**
 * g++ --std=c++11 -Wno-write-strings -O2 -g csound_threaded.cpp -ocsound_threaded -lcsound64 -lpthread -lm 
 */
int main(int argc, char *argv[])
{
    int inotifyFd, wd, j;
    char buf[BUF_LEN] __attribute__ ((aligned(8)));
    int numRead;
    char *p;
    struct inotify_event *event;

    if (argc != 3) {
      fprintf(stderr, "Usage: playa [music.csd] <log-dir>\n");
      exit(1);
    }

    char s[255];
    sprintf(s, "%s/stats.txt", argv[2]);

    FILE *fd = fopen(s, "r");
    if (fd == nullptr) {
      fprintf(stderr, "Requires a file in log-dir called stats.txt\n");
      exit(2);
    }

    /* Create inotify instance */
    inotifyFd = inotify_init();
    if (inotifyFd == -1) {
      printf("inotify_init\n");
      exit(2);
    }
  
    wd = inotify_add_watch(inotifyFd, argv[2], IN_ALL_EVENTS);
    if (wd == -1) {
      printf("inotify_add_watch\n");
      exit(3);
    }
    printf("Watching %s using wd %d\n", argv[2], wd);

    std::ifstream t;
    t.open(argv[1]);
    std::stringstream buffer;
    buffer << t.rdbuf();

    int res;

    CsoundThreaded csound;
    res = csound.SetOption("-d");
    if (res != CSOUND_SUCCESS) {
      fprintf(stderr, "opt 1: %d", res);
      exit(2);
    }
    csound.SetOption("-odac");
    if (res != CSOUND_SUCCESS) {
      fprintf(stderr, "opt 2: %d", res);
      exit(3);
    }
    csound.CompileCsdText(buffer.str().c_str());
    csound.Start();
    int thread = csound.Perform();
    std::cout << "Performing in thread 0x" << std::hex << thread << "..." << std::endl;



    auto synth_start = std::chrono::steady_clock::now();
    auto ping_start = std::chrono::steady_clock::now();
    auto pong_start = std::chrono::steady_clock::now();
    auto misc_start = std::chrono::steady_clock::now();

    for (;;) {                                  /* Read events forever */
      numRead = read(inotifyFd, buf, BUF_LEN);
      if (numRead == 0) {
        printf("read() from inotify fd returned 0!\n");
        exit(4);
      }

      if (numRead == -1) {
        printf("read\n");
        exit(5);
      }

      /* Process all of the events in buffer returned by read() */
      for (p = buf; p < buf + numRead; ) {
        event = (struct inotify_event *) p;
        if (event->len > 0) {
          printf("name = %s\n", event->name);
        }


        if (event->mask & IN_MODIFY) {
          int key = event->name[1] % 3;
          //csound.ScoreEvent('i', pcubefields[key], 7);
          if (strcmp(event->name, "dns.log") == 0) {
              auto play_time = std::chrono::steady_clock::now();
              auto time_span = play_time - ping_start;
              bool pass = time_span > one_second;
              if (pass) {
                double pfield [3] = {1, 0, 6};
                csound.ScoreEvent('i', pfield, 3);
                ping_start = play_time;
              }
          } else if (strcmp(event->name, "conn.log") == 0) {

              auto play_time = std::chrono::steady_clock::now();
              auto time_span = play_time - pong_start;
              bool pass = time_span > one_second;
              if (pass) {
                double pfield [3] = {2, 0, 6};
                csound.ScoreEvent('i', pfield, 3);
                pong_start = play_time;
              }
          } else if (strcmp(event->name, "stats.txt") == 0) {
            static const long max_len = 6 + 1;
            char buf[max_len + 1];

            fseek(fd, -max_len, SEEK_END);
            ssize_t len = fread(buf, 1, max_len, fd);

            buf[len] = '\0';
            char *last_newline = strrchr(buf, '\n');
            char *last_line = last_newline+1;

            int num_pkts = atoi(buf);
            printf("num_pkts=%d\n", num_pkts);

            old_p[samples % len] = num_pkts;
            samples ++;
            compute_new_avg(num_pkts);
            printf("moving avg: %f\n", weighted_m_avg);
            bool ramp = false;


            bool refrain = false;
            double pfield [8] = {12, 0, 2, 44, 1, 3, lforates[level], 0};
            //if (!ramp && num_pkts > weighted_m_avg*2) {
              refrain = true;
              double r = (double)(rand() % 100);
              pfield[7] = r;
            //}

            if (num_pkts > weighted_m_avg*2) {
              if(level != 3) {
                  level++;
                  ramp = true;
                  pfield[6] = lforates[level]/2; 
                  pfield[5] = 2; 
              }
            } else if (num_pkts < weighted_m_avg*0.10) {
              if(level != 0) {
                  level--;
                  ramp = true;
                  pfield[6] = lforates[level]/2; 
                  pfield[5] = 2; 
              }
            }

            auto play_time = std::chrono::steady_clock::now();
            auto time_span = play_time - synth_start;
            bool pass = time_span > two_second;

            //if (pass && ( ramp || refrain)) {
            if (pass && num_pkts != 0) {
              csound.ScoreEvent('i', pfield, 8);
              synth_start = play_time;
            }

            //double pfield [3] = {4, 0, 6};
            //csound.ScoreEvent('i', pfield, 3);

          } else {
              auto play_time = std::chrono::steady_clock::now();
              auto time_span = play_time - misc_start;
              bool pass = time_span > one_second;
              if (pass) {
                int r = rand() % 2 + 2; // play a G or a D, or a lowC
                double pfield [3] = {r, 0, 6};
                csound.ScoreEvent('i', pfield, 3);
                misc_start = play_time;
              }
          }
        }

        p += sizeof(struct inotify_event) + event->len;
      }
    }

    csound.Join();
    std::cout << "Finished." << std::endl;
}