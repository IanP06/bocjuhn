class TonitorOutput extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ring = new Float32Array(32768 * 2);
    this.write = 0; this.read = 0; this.pending = false;
    this.port.onmessage = ({data}) => {
      if (data.type === 'audio') {
        const a = data.samples;
        for (let i = 0; i < a.length / 2; i++) {
          const at = (this.write++ & 32767) * 2;
          this.ring[at] = a[i * 2]; this.ring[at + 1] = a[i * 2 + 1];
        }
        this.pending = false;
      } else if (data.type === 'clear') this.read = this.write;
    };
  }
  process(inputs, outputs) {
    const out = outputs[0], ratio = 44100 / sampleRate;
    for (let i = 0; i < out[0].length; i++) {
      if (this.write - this.read > 1) {
        const base = Math.floor(this.read), f = this.read - base;
        for (let ch = 0; ch < out.length; ch++) {
          const a = this.ring[(base & 32767) * 2 + ch];
          const b = this.ring[((base + 1) & 32767) * 2 + ch];
          out[ch][i] = Math.max(-1, Math.min(1, a + (b - a) * f));
        }
        this.read += ratio;
      } else { for (const channel of out) channel[i] = 0; }
    }
    if (this.write - this.read < 1536 && !this.pending) {
      this.pending = true; this.port.postMessage({type:'need'});
    }
    return true;
  }
}
registerProcessor('tonitor-output', TonitorOutput);
