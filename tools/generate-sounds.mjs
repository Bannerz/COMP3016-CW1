//Original procedural game effects. Regenerate with: node tools/generate-sounds.mjs
import { mkdirSync, writeFileSync } from 'node:fs';
import { fileURLToPath } from 'node:url';

const rate = 48000;
const tau = Math.PI * 2;
const directory = fileURLToPath(new URL('../src/assets/audio/', import.meta.url));
mkdirSync(directory, { recursive: true });
let seed = 3016;
function noise() {
    seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0;
    return seed / 2147483648 - 1;
}
const sine = (cycles) => Math.sin(tau * cycles);
function effect(seconds, synth, loop = false) {
    const samples = Float64Array.from({ length: Math.round(seconds * rate) }, (_, i) => synth(i / rate));
    if (loop) {
        //Overlap the tail and head, preserving the texture at the loop boundary.
        const overlap = Math.round(rate * 0.06);
        const length = samples.length - overlap;
        for (let i = 0; i < overlap; ++i) {
            const blend = i / overlap;
            samples[i] = samples[length + i] * (1 - blend) + samples[i] * blend;
        }
        return samples.slice(0, length);
    }
    for (let i = 0; i < samples.length; ++i) {
        const t = i / rate;
        samples[i] *= Math.min(1, t / 0.004, (seconds - t) / 0.025);
    }
    return samples;
}
function save(name, samples) {
    //Remove DC and leave headroom; short effects can overlap in the mixer.
    const mean = samples.reduce((a, b) => a + b, 0) / samples.length;
    let peak = 0;
    for (let i = 0; i < samples.length; ++i) {
        samples[i] -= mean;
        peak = Math.max(peak, Math.abs(samples[i]));
    }
    const buffer = Buffer.alloc(44 + samples.length * 2);
    buffer.write('RIFF', 0); buffer.writeUInt32LE(buffer.length - 8, 4);
    buffer.write('WAVEfmt ', 8); buffer.writeUInt32LE(16, 16);
    buffer.writeUInt16LE(1, 20); buffer.writeUInt16LE(1, 22);
    buffer.writeUInt32LE(rate, 24); buffer.writeUInt32LE(rate * 2, 28);
    buffer.writeUInt16LE(2, 32); buffer.writeUInt16LE(16, 34);
    buffer.write('data', 36); buffer.writeUInt32LE(samples.length * 2, 40);
    for (let i = 0; i < samples.length; ++i) {
        buffer.writeInt16LE(Math.round(samples[i] / Math.max(peak, 0.001) * 24575), 44 + i * 2);
    }
    writeFileSync(directory + name + '.wav', buffer);
    console.log(`${name}: ${(samples.length / rate).toFixed(2)} s, peak -2.5 dBFS`);
}

let low = 0;
save('launcher-pull', effect(1.5, t => {
    low += 0.06 * (noise() - low);
    const friction = 0.6 + 0.22 * sine(7 * t) + 0.18 * sine(19 * t);
    const phase = 126 * t + 0.7 * sine(3 * t) + 0.12 * sine(31 * t);
    return friction * (0.43 * sine(phase) + 0.14 * sine(phase * 2.03)) + low * 0.8;
}, true));

//Short, soft UI cues stay separate from the brighter reward chime.
save('button-hover', effect(0.075, t => {
    const phase = 680 * t + 180 * 0.015 * (1 - Math.exp(-t / 0.015));
    return (sine(phase) + 0.12 * sine(phase * 2)) * Math.exp(-t * 65);
}));

save('button-click', effect(0.13, t => {
    const phase = 340 * t + 430 * 0.012 * (1 - Math.exp(-t / 0.012));
    return (sine(phase) + 0.20 * sine(phase * 2)) * Math.exp(-t * 38);
}));

save('launcher-release', effect(0.52, t => {
    const phase = 92 * t + 235 * 0.052 * (1 - Math.exp(-t / 0.052));
    const twang = (sine(phase + 0.7 * Math.exp(-t * 8) * sine(22 * t)) + 0.25 * sine(phase * 2.02)) * Math.exp(-t * 11);
    const swish = noise() * (1 - Math.exp(-t * 160)) * Math.exp(-t * 26);
    return 0.7 * twang + 0.33 * swish;
}));

save('coin-collect', effect(0.48, t => {
    let chime = 0;
    for (const [delay, hz, gain] of [[0, 1318.51, 0.7], [0.055, 1975.53, 0.5], [0.10, 2637.02, 0.22]]) {
        const u = t - delay;
        if (u >= 0) chime += gain * (1 - Math.exp(-u * 800)) * Math.exp(-u * 17) *
            (sine(hz * u) + 0.17 * sine(hz * 2.76 * u) * Math.exp(-u * 35));
    }
    return chime;
}));

save('trampoline-bounce', effect(0.8, t => {
    const phase = 115 * t + 260 * 0.085 * (1 - Math.exp(-t / 0.085)) + 0.9 * sine(11 * t) * Math.exp(-t * 5);
    const spring = (sine(phase) + 0.22 * sine(phase * 2) + 0.08 * sine(phase * 3.03)) * Math.exp(-t * 6.5);
    const mat = 0.18 * noise() * Math.exp(-t * 65) + 0.3 * sine(72 * t) * Math.exp(-t * 25);
    return spring * 0.75 + mat;
}));

low = 0;
save('ground-impact', effect(0.42, t => {
    low += 0.15 * (noise() - low);
    const thud = sine(57 * t + 100 * 0.018 * (1 - Math.exp(-t / 0.018))) * Math.exp(-t * 22);
    return 0.85 * thud + 0.7 * low * Math.exp(-t * 30) + 0.10 * noise() * Math.exp(-t * 90);
}));

low = 0;
let slower = 0;
save('ground-roll', effect(1.8, t => {
    low += 0.11 * (noise() - low);
    slower += 0.014 * (low - slower);
    const texture = (low - slower) * (0.7 + 0.12 * sine(13 * t) + 0.18 * sine(29 * t));
    const rumble = 0.12 * sine(61 * t + 0.25 * sine(4 * t));
    return texture * 1.8 + rumble;
}, true));
