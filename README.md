# 6502
Minimal implementation of 6502 CPU in C with 0 dependencies


Everything except decimal mode works

`nestest` : works

`6502_functional_test.bin` : fails adc test (# 2A)

log:

```
26764011 336A  8D 00 02  STA $0200
```

```
3472  65 0E     ADC $0E = 99  | A:99 X:0E Y:FF P:E9 SP:FC CYC:329
3474  08        PHP           | A:33 X:0E Y:FF P:69 SP:FC CYC:338
3475  C5 0F     CMP $0F = 99  | A:33 X:0E Y:FF P:69 SP:FB CYC:  6
3477  D0 FE     BNE $3477     | A:33 X:0E Y:FF P:E8 SP:FB CYC: 15
3477  D0 FE     BNE $3477     | A:33 X:0E Y:FF P:E8 SP:FB CYC: 24
3477  D0 FE     BNE $3477     | A:33 X:0E Y:FF P:E8 SP:FB CYC: 33
```
