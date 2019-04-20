#include <stdio.h>
#include <string.h>
#include <sndfile.h>
#include <getopt.h>
#include <stdlib.h>

/* Copyright (c) 2003 Lars Magne Ingebrigtsen <larsi@gnus.org>.
   Released under the GNU General Public License. */
  
struct option long_options[] = {
  {"length", 1, 0, 'l'},
  {"silence", 1, 0, 's'},
  {"help", 0, 0, 'h'},
  {"pattern", 1, 0, 'p'},
  {0, 0, 0, 0}
};

#define MAX_FILE_NAME 10240

static char *output_pattern = "unsilence-%02d.wav";
static int silence_length = 60;
static int silence_limit = 10;

void print_usage(void) {
  printf("Usage: unsilence [--length <silence in seconds>]\n");
  printf("                 [--pattern <output file pattern>]\n");
  printf("                 [--silence <silent value>]\n");
  printf("                 <input file>\n");
}

int parse_args(int argc, char **argv) {
  int option_index = 0, c;

  while (1) {
    c = getopt_long(argc, argv, "hl:p:", long_options, &option_index);
    if (c == -1)
      break;

    switch (c) {

    case 'l':
      silence_length = atoi(optarg);
      break;

    case 's':
      silence_limit = atoi(optarg);
      break;

    case 'p':
      output_pattern = optarg;
      break;

    case 'h':
      print_usage();
      exit(-1);
      break;

    default:
      break;
    }
  }

  return optind;
}

void write_file(short *buffer, int part, int start, int stop, SF_INFO *info) {
  SF_INFO output_info;
  SNDFILE* output;
  char output_file_name[MAX_FILE_NAME];
  int length = stop - start;

  bzero(&output_info, sizeof(SF_INFO));
  
  output_info.samplerate = info->samplerate;
  output_info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
  output_info.channels = info->channels;

  snprintf(output_file_name, MAX_FILE_NAME, output_pattern, part);

  printf("Writing %s (%d:%d (%02d:%02d))...\n", output_file_name, start, stop,
	 length/info->samplerate/60, (length/info->samplerate)%60);
  
  if ((output = sf_open(output_file_name, SFM_WRITE, &output_info)) == NULL) {
    sf_perror(output);
    exit(-1);
  }

  if (sf_writef_short(output, buffer+start*info->channels, length) != length) {
    sf_perror(output);
    exit(-1);
  }

  sf_close(output);
}

int unsilence(const char *input_file_name) {
  SNDFILE* input;
  SF_INFO input_info;
  short *buffer;
  int frames, channels;
  int start_silence = -1, start_noise = -1;
  int part = 0, i, max = 0;

  if ((input = sf_open(input_file_name, SFM_READ, &input_info)) == NULL) {
    sf_perror(input);
    exit(-1);
  }

  frames = input_info.frames;
  channels = input_info.channels;

  printf("File: %s, length: %d:%02d; frames %d, channels %d\n",
	 input_file_name,
	 frames/input_info.samplerate/60, (frames/input_info.samplerate)%60,
	 frames,
	 channels);
  
  buffer = (short*) malloc(frames * channels * sizeof(short));
  if (sf_readf_short(input, buffer, frames) != frames) {
    perror("unsilence (reading)");
    exit(-1);
  }

  for (i = 0; i <= frames; i++) {
    if (i < frames &&
	abs(buffer[i * 2]) < silence_limit &&
	abs(buffer[i * 2 + 1]) < silence_limit) {
      if (start_silence == -1)
	start_silence = i;

      if (i - start_silence > max)
	max = i - start_silence;
    } else {
      if (start_silence != -1 &&
	  start_noise != -1 &&
	  ((i == frames && part > 0) ||
	   (i - start_silence) / input_info.samplerate >
	   silence_length)) {
	write_file(buffer, ++part, start_noise, start_silence, &input_info);
	start_noise = -1;
      }
      
      if (start_noise == -1)
	start_noise = i;

      start_silence = -1;
    }
  }
  
  sf_close(input);
  free(buffer);
  
  printf("Max silence: %02d:%02d\n",
	 max/input_info.samplerate/60, (max/input_info.samplerate)%60);

  return part;
}

int main(int argc, char **argv)
{
  int dirn, parts;

  dirn = parse_args(argc, argv);
  
  if (dirn == argc) {
    print_usage();
    exit(-1);
  }

  parts = unsilence(argv[dirn]);

  if (parts > 0)
    exit(0);
  else
    exit(-2);
}


