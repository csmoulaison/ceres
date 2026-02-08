#include <alsa/asoundlib.h>
#include <alloca.h>

#define ALSA_SAMPLE_RATE 48000
#define ALSA_LATENCY_SAMPLES ALSA_SAMPLE_RATE / 10
#define ALSA_MAX_BUFFER_SAMPLES 8196

#define ALSA_VERIFY(alsa_function) { \
	i32 alsa_error; \
	if((alsa_error = alsa_function) < 0) { \
		fprintf(stderr, "ALSA error: %s\n", snd_strerror(alsa_error)); \
		exit(1); \
	} \
} 

typedef struct {
	snd_pcm_t* pcm;
	i16 write_buffer[ALSA_MAX_BUFFER_SAMPLES * 2];
} AlsaMemory;

void alsa_init(AlsaMemory* alsa) {
	snd_pcm_hw_params_t* hw_params;

	ALSA_VERIFY(snd_pcm_open(&alsa->pcm, "default", SND_PCM_STREAM_PLAYBACK, 0));
	snd_pcm_hw_params_alloca(&hw_params);
	ALSA_VERIFY(snd_pcm_hw_params_any(alsa->pcm, hw_params));
	ALSA_VERIFY(snd_pcm_hw_params_set_access(alsa->pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED));
	ALSA_VERIFY(snd_pcm_hw_params_set_format(alsa->pcm, hw_params, SND_PCM_FORMAT_S16_LE));
	u32 sample_rate = ALSA_SAMPLE_RATE;
	ALSA_VERIFY(snd_pcm_hw_params_set_rate_near(alsa->pcm, hw_params, &sample_rate, 0));
	ALSA_VERIFY(snd_pcm_hw_params_set_channels(alsa->pcm, hw_params, 2));
	ALSA_VERIFY(snd_pcm_hw_params(alsa->pcm, hw_params));
	ALSA_VERIFY(snd_pcm_prepare(alsa->pcm));
}

i32 alsa_write_samples_count(AlsaMemory* alsa) {
	snd_pcm_sframes_t available;
	snd_pcm_sframes_t delay;
	ALSA_VERIFY(snd_pcm_avail_delay(alsa->pcm, &available, &delay));

	// TODO: Make sure we have enough frames available.
	return ALSA_LATENCY_SAMPLES - delay;
}

void alsa_write_samples(AlsaMemory* alsa, i16* buffer, i32 sample_count) {
	assert(snd_pcm_writei(alsa->pcm, buffer, sample_count) == sample_count);
}
