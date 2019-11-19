

# 未经过编码的PCM数据播放
ar: 采样率44.1KHz
ac: 音频声道channel 双声道
f:  采样位 16位

ffplay -ar 44100 -ac 2 -f s16le output.pcm