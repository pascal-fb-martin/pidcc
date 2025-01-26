#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pigpio.h> // Raspberry Pi OS only, not on regular Debian.
#include <sys/select.h>

static int gpioa;
static int gpiob;

static void oneShotWave (int count, gpioPulse_t *pulses, int option) {

  if (gpioWaveAddNew()) {
     printf ("gpioWaveAddNew() failed\n");
     gpioTerminate ();
     exit (1);
  }

  int result = gpioWaveAddGeneric(count, pulses);
  if (result < 0) {
     printf ("gpioWaveAddGeneric() failed, error %d\n", result);
     gpioTerminate ();
     exit (1);
  }

  int wave = gpioWaveCreate();
  if (wave < 0) {
     printf ("gpioWaveCreate() failed\n");
     gpioTerminate ();
     exit (1);
  }
  result = gpioWaveTxSend (wave, option);
  if (result < 0) {
     printf ("gpioWaveTxSend() failed, result %d\n", result);
     gpioTerminate ();
     exit (1);
  }

  while (gpioWaveTxBusy()) {
    struct timeval timer;
    timer.tv_sec = 0;
    timer.tv_usec = 500;
    select (0, 0, 0, 0, &timer);
  }
  gpioWaveDelete (wave);
}

static void startStopWave (int count, gpioPulse_t *pulses) {

  gpioPulse_t start[4];
  gpioPulse_t stop[4];

  start[0].gpioOn = (1 << gpioa);
  start[0].gpioOff = gpiob ? (1 << gpiob) : 0;
  start[0].usDelay = 40;
  start[1].gpioOn = gpiob ? (1 << gpiob) : 0;
  start[1].gpioOff = (1 << gpioa);
  start[1].usDelay = 20;
  start[2].gpioOn = (1 << gpioa);
  start[2].gpioOff = gpiob ? (1 << gpiob) : 0;
  start[2].usDelay = 40;
  start[3].gpioOn = (1 << gpioa) + (gpiob ? (1 << gpiob) : 0);
  start[3].gpioOff = 0;
  start[3].usDelay = 60;

  stop[0].gpioOn = (1 << gpioa);
  stop[0].gpioOff = gpiob ? (1 << gpiob) : 0;
  stop[0].usDelay = 40;
  stop[1].gpioOn = gpiob ? (1 << gpiob) : 0;
  stop[1].gpioOff = (1 << gpioa);
  stop[1].usDelay = 60;
  stop[2].gpioOn = (1 << gpioa);
  stop[2].gpioOff = gpiob ? (1 << gpiob) : 0;
  stop[2].usDelay = 60;

  int t;
  for (t = 0; t < 300000; t++) {
    oneShotWave (4, start, 0); // isolated 20us pulse, followed by all up.
    oneShotWave (count, pulses, 0);
    oneShotWave (3, stop, 0);  // isolated 40us pulse.
  }
}

int main (int argc, const char **argv) {

  if (argc <= 1) {
     printf ("GPIO number is missing\n");
     exit(1);
  }

  if (!strcmp (argv[1], "-h")) {
     printf ("%s [-h] gpioa[':' gpiob] pulse ..\n\n", argv[0]);
     printf ("  Repeatedly generate the specified pulse sequence, between\n");
     printf ("  a 20 usec start pulse and a 60 usec stop pulse.\n");
     exit (0);
  }

  gpioa = atoi(argv[1]);
  const char *sep = strchr (argv[1], ':');
  gpiob = sep ? atoi(sep+1) : 0;

  int i;
  gpioPulse_t pulses[10];

  if (argc > 12) argc = 12;

  for (i = 2; i < argc; ++i) {
     if (i & 1) {
        pulses[i-2].gpioOn = (1 << gpioa);
        pulses[i-2].gpioOff = gpiob ? (1 << gpiob) : 0;
     } else {
        pulses[i-2].gpioOn = gpiob ? (1 << gpiob) : 0;
        pulses[i-2].gpioOff = (1 << gpioa);
     }
     pulses[i-2].usDelay = atoi(argv[i]);
  }
  int count = argc - 2;

  if (gpioInitialise() < 0) {
     printf ("pigio initialization failed\n");
     exit (1);
  }
  if (gpioSetMode(gpioa, PI_OUTPUT)) {
     printf ("gpioSetMode(gpioa) failed\n");
     gpioTerminate ();
     exit (1);
  }
  if (sep) {
     if (gpioSetMode(gpiob, PI_OUTPUT)) {
        printf ("gpioSetMode(gpiob) failed\n");
        gpioTerminate ();
        exit (1);
     }
  }

  startStopWave (count, pulses);

  gpioTerminate ();
}

