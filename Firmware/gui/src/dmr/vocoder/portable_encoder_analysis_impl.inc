/* Internal fixed-point encoder analysis implementation. */


void vocoder_analyze_encoder_frame_raw(intptr_t argument_1,intptr_t argument_2,int argument_3,intptr_t argument_4,
                 int32_t argument_5,intptr_t argument_6,short argument_7,short *argument_8,
                 intptr_t argument_9)

{
  uint32_t unsigned_value_1;
  int64_t wide_value_2;
  bool condition_3;
  short short_value_4;
  short short_value_5;
  uint16_t unsigned_value_6;
  short short_value_7;
  int signed_value_8;
  uint32_t unsigned_value_9;
  intptr_t signed_value_10;
  short *short_pointer_11;
  intptr_t signed_value_12;
  uint32_t *unsigned_pointer_13;
  uint32_t *unsigned_pointer_14;
  int signed_value_15;
  uint32_t unsigned_value_16;
  uint32_t *unsigned_pointer_17;
  uint32_t unsigned_value_18;
  short short_value_19;
  uint32_t *unsigned_pointer_20;
  uint32_t *unsigned_pointer_21;
  intptr_t signed_value_23;
  uint16_t unsigned_value_24;
  intptr_t signed_value_25;
  short *short_pointer_26;
  short short_value_27;
  intptr_t signed_value_28;
  uint8_t *unsigned_pointer_29;
  uint32_t unsigned_value_30;
  uint32_t unsigned_value_31;
  bool condition_32;
  int work_78c;
  int work_788;
  int work_784;
  intptr_t work_780;
  uint8_t *work_77c;
  int work_774;
  int work_770;
  uint32_t *work_764;
  uint8_t *work_760;
  intptr_t work_75c;
  int work_758;
  intptr_t work_754;
  uint32_t work_744;
  int work_738;
  int work_734;
  union {
    uint32_t alignment;
    uint8_t bytes[0x784];
  } frame_storage;
  uint32_t *work_704 = (uint32_t *)(void *)(frame_storage.bytes + 0x0a4);
  uint8_t *workspace_pointer_504 = frame_storage.bytes + 0x2a4;
  uint32_t *work_4c4 = (uint32_t *)(void *)(frame_storage.bytes + 0x2e4);
  uint8_t *workspace_pointer_2c0 = frame_storage.bytes + 0x4e8;
  uint32_t *work_160 = (uint32_t *)(void *)(frame_storage.bytes + 0x648);
  uint32_t *work_e0 = (uint32_t *)(void *)(frame_storage.bytes + 0x6c8);
  uint8_t *workspace_pointer_b8 = frame_storage.bytes + 0x6f0;
  uint32_t *workspace_pointer_a4 = (uint32_t *)(void *)(frame_storage.bytes + 0x704);
  uint32_t *work_60 = (uint32_t *)(void *)(frame_storage.bytes + 0x748);
  uint32_t *work_40 = (uint32_t *)(void *)(frame_storage.bytes + 0x768);
  int *work_2c = (int *)(void *)(frame_storage.bytes + 0x77c);
  uint32_t *unsigned_pointer_22;
  
  signed_value_8 = vocoder_analyze_subframes(argument_1,workspace_pointer_2c0,workspace_pointer_504,work_40 + 2,argument_4,0x17f,argument_5)
  ;
  vocoder_update_spectral_energy_state
            (argument_6,workspace_pointer_504,*(short *)(argument_1 + 0x62c) * 2 + 7,work_40[2]);
  short_value_5 = (short)signed_value_8;
  if (0 < argument_3) {
    work_40[3] = -0x1e;
    for (signed_value_10 = 0; signed_value_10 < 32; signed_value_10 = signed_value_10 + 1) {
      work_160[signed_value_10] = 0;
    }
    unsigned_value_9 = vocoder_max_i16(argument_1 + 0x620,7);
    short_value_19 = *(short *)(argument_6 + 0x72);
    short_value_27 = *(short *)(argument_6 + 0x52);
    work_2c[0] = 0;
    short_value_4 = 1;
    short_pointer_11 = (short *)(argument_6 + 0x54);
    do {
      signed_value_10 = vocoder_compare_scaled((int)*short_pointer_11,(int)short_pointer_11[0x10],(int)short_value_27,(int)short_value_19);
      short_value_4 = short_value_4 + 1;
      if (signed_value_10 != 0) {
        short_value_27 = *short_pointer_11;
        short_value_19 = short_pointer_11[0x10];
      }
      short_pointer_11 = short_pointer_11 + 1;
    } while (short_value_4 != 0x10);
    short_value_4 = 0;
    short_value_7 = 8;
    short_pointer_11 = (short *)(argument_6 + 0x14e);
    do {
      short_value_4 = vocoder_normalized_add((int)short_value_4,work_2c[0],(int)short_pointer_11[1],(int)short_pointer_11[0x11],work_2c
                                 );
      short_value_7 = short_value_7 + -1;
      short_pointer_11 = short_pointer_11 + 1;
    } while (short_value_7 != 0);
    short_value_7 = *(short *)(argument_6 + 0x180);
    signed_value_25 = (int)short_value_4;
    signed_value_10 = vocoder_compare_scaled(signed_value_25,work_2c[0],0x7a12,0xfffffff6);
    if (signed_value_10 == 0) {
      signed_value_10 = vocoder_compare_scaled(signed_value_25,work_2c[0],0xc35,0xfffffff6);
      if ((signed_value_10 != 0) &&
         (signed_value_10 = vocoder_compare_scaled(signed_value_25,work_2c[0],
                                       *(short *)(argument_6 + 0x13c) * 0x4666 >> 0xf,
                                       (int)*(short *)(argument_6 + 0x13e)), signed_value_10 != 0)) {
        short_value_7 = 1;
        goto encoder_branch_01;
      }
      if (((-0x1e00 < *(short *)(argument_6 + 0x50)) ||
          (signed_value_10 = vocoder_compare_scaled((int)short_value_27,(int)short_value_19,0xffffb000,5), signed_value_10 != 0)) ||
         ((int)*(short *)(argument_6 + 0x48) - (int)*(short *)(argument_6 + 0x50) < 0xf00))
      goto encoder_branch_01;
      signed_value_10 = 0;
      condition_3 = false;
      work_738 = 0;
    }
    else {
      short_value_7 = 1;
encoder_branch_01:
      short_value_19 = 8;
      wide_value_2 = 0;
      short_pointer_11 = (short *)(argument_6 + 0x50);
      do {
        while( true ) {
          short_pointer_26 = short_pointer_11 + 1;
          signed_value_10 = (int)short_pointer_11[0x11];
          short_pointer_11 = short_pointer_26;
          if (4 < signed_value_10) break;
          short_value_19 = short_value_19 + -1;
          wide_value_2 = (int32_t)ENCODER_ASR32(
              ENCODER_LSL32((int)*short_pointer_26,0x10),5U - signed_value_10) + wide_value_2;
          if (short_value_19 == 0) goto encoder_branch_02;
        }
        short_value_19 = short_value_19 + -1;
        wide_value_2 = (int32_t)ENCODER_LSL32(
            ENCODER_LSL32((int)*short_pointer_26,0x10),signed_value_10 - 5U) + wide_value_2;
      } while (short_value_19 != 0);
encoder_branch_02:
      signed_value_10 = (int32_t)ENCODER_ASR32(
          (uint32_t)wide_value_2 >> 3 | ENCODER_LSL32(
              (uint32_t)((uint64_t)wide_value_2 >> 0x20),0x1d),0x10);
      if (signed_value_10 < -0x7000) {
        signed_value_10 = -0x7000;
      }
      work_738 = 5;
      condition_3 = true;
    }
    signed_value_25 = (int)short_value_5;
    work_75c = argument_1 + (signed_value_25 + -9) * 2;
    work_754 = argument_6;
    work_760 = workspace_pointer_2c0;
    work_774 = 0;
    unsigned_pointer_20 = work_704;
    work_764 = work_60;
    do {
      work_78c = 0;
      work_780 = work_75c;
      work_770 = work_774 << 1;
      work_77c = work_760;
      work_788 = 0;
      work_744 = 0;
      work_734 = 0;
      work_784 = 0;
      unsigned_pointer_14 = unsigned_pointer_20;
      do {
        vocoder_copy_i16(work_4c4,work_780,(int)(short)(0x3a - signed_value_25));
        vocoder_copy_i16((intptr_t)work_4c4 + (0x3a - signed_value_25) * 2,work_77c,signed_value_25);
        signed_value_28 = 6;
        short_pointer_11 = (short *)(argument_1 + 0x62c);
        do {
          signed_value_15 = (int)short_pointer_11[1];
          signed_value_12 = (intptr_t)work_4c4 + signed_value_15 * 2;
          vocoder_shift_i16_between_exponents(signed_value_12,signed_value_12,short_pointer_11[2] - signed_value_15,(int)short_pointer_11[-6],unsigned_value_9)
          ;
          signed_value_28 = signed_value_28 + -1;
          short_pointer_11 = short_pointer_11 + 1;
        } while (signed_value_28 != 0);
        signed_value_28 = (intptr_t)work_4c4 + *(short *)(argument_1 + 0x63a) * 2;
        vocoder_shift_i16_between_exponents
                  (signed_value_28,signed_value_28,0x3a - *(short *)(argument_1 + 0x63a),(int)*(short *)(argument_1 + 0x62c)
                   ,unsigned_value_9);
        signed_value_28 = vocoder_windowed_real_fft_energy
                           (unsigned_pointer_14,work_4c4,unsigned_value_9,encoder_window_58,0,0x3a,0x1d,work_4c4,6,0);
        unsigned_pointer_14[1] = 0;
        *(int *)((intptr_t)work_40 + work_788) = (int)signed_value_28;
        *unsigned_pointer_14 = 0;
        if (condition_3) {
          unsigned_pointer_13 = work_e0;
          unsigned_pointer_17 = unsigned_pointer_14;
          if (work_774 == 0) {
            signed_value_12 = (int32_t)(
                (uint32_t)(int32_t)*(short *)(argument_6 + 0x136) * 0x8000U +
                (uint32_t)(int32_t)*(short *)(argument_6 + 0x134) * 0x8000U);
            if (work_784 == 1) {
              signed_value_12 = ((*(short *)(argument_6 + 0x122) * 0x10000 +
                        *(short *)(argument_6 + 0x112) * 0x10000) - (signed_value_12 >> 1)) * 2;
            }
            if ((int32_t)((uint32_t)signed_value_12 - 0x12000000U) < 0) {
              wide_value_2 = (int64_t)(int32_t)((uint32_t)signed_value_12 - 0x12000000U) * 0x5560;
              unsigned_value_16 = vocoder_exp2_q16((int32_t)ENCODER_ASR32(
                  (uint32_t)wide_value_2 >> 0xf | ENCODER_LSL32(
                      (uint32_t)((uint64_t)wide_value_2 >> 0x20),0x11),9),work_2c);
              unsigned_value_31 = unsigned_value_16;
              if ((int)unsigned_value_16 < 0) {
                unsigned_value_31 = ~unsigned_value_16;
              }
              short_value_19 = (short)unsigned_value_31;
              if (unsigned_value_16 != 0) {
                short_value_19 = (short)COUNT_LEADING_ZEROS(unsigned_value_31) + -1;
              }
              unsigned_value_16 = unsigned_value_16 << ((int)short_value_19 & 0xffU);
              signed_value_15 = work_2c[0] - short_value_19;
              work_744 = unsigned_value_16 >> 0x10;
              short_value_19 = (short)(unsigned_value_16 >> 0x10);
              signed_value_12 = vocoder_compare_scaled_less((int)short_value_19,signed_value_15,0x51eb,0xfffffffa);
              if (signed_value_12 == 0) {
                unsigned_value_16 = *unsigned_pointer_14;
                unsigned_value_31 = (uint32_t)short_value_19;
                signed_value_12 = (int)unsigned_value_31 >> 0x1f;
                work_734 = (int)(short)signed_value_15;
                signed_value_15 = work_734;
              }
              else {
                unsigned_value_16 = *unsigned_pointer_14;
                work_744 = 0x51eb;
                unsigned_value_31 = 0x51eb;
                signed_value_12 = 0;
                work_734 = -6;
                signed_value_15 = work_734;
              }
            }
            else {
              unsigned_value_16 = 0;
              unsigned_value_31 = 0x4000;
              signed_value_12 = 0;
              work_734 = 1;
              work_744 = 0x4000;
              signed_value_15 = work_734;
            }
          }
          else {
            signed_value_23 = work_754 + work_78c;
            signed_value_12 = (int32_t)ENCODER_LSL32(
                         (uint32_t)work_770 * (uint32_t)-0x1111000 + 0x10000000U,
                         5U - work_738) +
                     signed_value_10 * 0x10000;
            signed_value_15 = (int32_t)ENCODER_LSL32(
                (uint32_t)work_770 * (uint32_t)-0x2222000 - 0x30000000U,
                5U - work_738);
            if (signed_value_15 <= signed_value_12) {
              signed_value_12 = signed_value_15;
            }
            signed_value_15 = (int32_t)ENCODER_LSL32(
                *(short *)(signed_value_23 + 0x52),0x10);
            unsigned_value_31 = *(short *)(signed_value_23 + 0x72) - work_738;
            if ((int)unsigned_value_31 < 0) {
              signed_value_15 = signed_value_15 >> (work_738 - *(short *)(signed_value_23 + 0x72) & 0xffU);
            }
            else {
              signed_value_15 = (int32_t)ENCODER_LSL32(
                  (uint32_t)signed_value_15, unsigned_value_31);
            }
            if ((short_value_7 != 0) && (8 < work_770)) {
              unsigned_value_16 = vocoder_log2_q15((int)*(short *)(signed_value_23 + 0x140),(int)*(short *)(signed_value_23 + 0x160)
                                    );
              unsigned_value_31 = unsigned_value_16;
              if ((int)unsigned_value_16 < 0) {
                unsigned_value_31 = ~unsigned_value_16;
              }
              short_value_19 = (short)unsigned_value_31;
              if (unsigned_value_16 != 0) {
                short_value_19 = (short)COUNT_LEADING_ZEROS(unsigned_value_31) + -1;
              }
              unsigned_value_31 = (short)(0xf - short_value_19) - work_738;
              unsigned_value_16 = unsigned_value_16 << ((int)short_value_19 & 0xffU);
              if ((int)unsigned_value_31 < 1) {
                unsigned_value_30 = (int)unsigned_value_16 >> (work_738 - (short)(0xf - short_value_19) & 0xffU);
              }
              else {
                unsigned_value_30 = unsigned_value_16 << (unsigned_value_31 & 0xff);
                unsigned_value_31 = (int)unsigned_value_30 >> (unsigned_value_31 & 0xff);
                condition_32 = unsigned_value_16 != unsigned_value_31;
                if (condition_32) {
                  unsigned_value_31 = 0x7fffffff;
                }
                if (condition_32) {
                  unsigned_value_30 = unsigned_value_31 ^ (int)unsigned_value_16 >> 0x1f;
                }
              }
              if (signed_value_15 >> 0x10 < (int)unsigned_value_30 >> 0x10) {
                signed_value_15 = ((int)unsigned_value_30 >> 0x10) << 0x10;
              }
            }
            unsigned_value_31 = signed_value_12 - signed_value_15;
            if ((int)unsigned_value_31 < 0) {
              unsigned_value_16 = vocoder_exp2_q16((int32_t)ENCODER_ASR32(
                  (unsigned_value_31 & 0x3ffff) >> 1 | ENCODER_LSL32(
                      ENCODER_LSL32((int)unsigned_value_31 >> 0x1f,0xe) |
                      unsigned_value_31 >> 0x12,0x11),9),work_2c);
              unsigned_value_31 = unsigned_value_16;
              if ((int)unsigned_value_16 < 0) {
                unsigned_value_31 = ~unsigned_value_16;
              }
              short_value_19 = (short)unsigned_value_31;
              if (unsigned_value_16 != 0) {
                short_value_19 = (short)COUNT_LEADING_ZEROS(unsigned_value_31) + -1;
              }
              unsigned_value_31 = (uint32_t)(short)((unsigned_value_16 << ((int)short_value_19 & 0xffU)) >> 0x10);
              signed_value_12 = (int)unsigned_value_31 >> 0x1f;
              unsigned_value_16 = *unsigned_pointer_14;
              signed_value_15 = (int)(short)((short)work_2c[0] - short_value_19);
            }
            else {
              unsigned_value_31 = 0x4000;
              signed_value_12 = 0;
              unsigned_value_16 = *unsigned_pointer_14;
              signed_value_15 = 1;
            }
          }
          while( true ) {
            *unsigned_pointer_13 = (uint32_t)((uint64_t)unsigned_value_16 * (uint64_t)unsigned_value_31) >> 0xf |
                       (unsigned_value_31 * ((int)unsigned_value_16 >> 0x1f) + unsigned_value_16 * signed_value_12 +
                       (int)((uint64_t)unsigned_value_16 * (uint64_t)unsigned_value_31 >> 0x20)) * 0x20000;
            if (unsigned_pointer_13 + 1 == work_60) break;
            unsigned_value_16 = unsigned_pointer_17[1];
            unsigned_pointer_13 = unsigned_pointer_13 + 1;
            unsigned_pointer_17 = unsigned_pointer_17 + 1;
          }
          signed_value_28 = signed_value_28 + signed_value_15;
          if (work_774 == 0) {
            unsigned_value_31 = (uint32_t)(short)work_744;
            signed_value_12 = *(int *)((intptr_t)work_40 + work_788);
            unsigned_pointer_13 = unsigned_pointer_14 + 1;
            do {
              unsigned_value_16 = unsigned_pointer_13[-1];
              unsigned_pointer_13[-1] = (uint32_t)((uint64_t)unsigned_value_16 * (uint64_t)unsigned_value_31) >> 0xf |
                            (unsigned_value_31 * ((int)unsigned_value_16 >> 0x1f) + unsigned_value_16 * ((int)unsigned_value_31 >> 0x1f) +
                            (int)((uint64_t)unsigned_value_16 * (uint64_t)unsigned_value_31 >> 0x20)) * 0x20000;
              unsigned_pointer_13 = unsigned_pointer_13 + 1;
            } while (unsigned_pointer_13 != unsigned_pointer_14 + 0x21);
            *(int *)((intptr_t)work_40 + work_788) = (int)(signed_value_12 + work_734);
          }
          if (signed_value_28 <= (int)work_40[3]) goto encoder_branch_03;
encoder_branch_04:
          unsigned_pointer_14 = work_160;
          unsigned_pointer_13 = work_e0;
          signed_value_12 = work_40[3];
        }
        else {
          signed_value_12 = 0;
          do {
            *(uint32_t *)((intptr_t)work_e0 + signed_value_12) = *(uint32_t *)((intptr_t)unsigned_pointer_14 + signed_value_12);
            signed_value_12 = signed_value_12 + 4;
          } while (signed_value_12 != 0x80);
          if ((int)work_40[3] < signed_value_28) goto encoder_branch_04;
encoder_branch_03:
          unsigned_pointer_14 = work_e0;
          unsigned_pointer_13 = work_160;
          signed_value_12 = signed_value_28;
          signed_value_28 = work_40[3];
        }
        unsigned_pointer_17 = work_160;
        unsigned_pointer_21 = unsigned_pointer_13;
        do {
          unsigned_pointer_22 = unsigned_pointer_21 + 1;
          unsigned_value_31 = (uint32_t)encoder_saturating_add_i32(
              (int32_t)*unsigned_pointer_21,
              (int32_t)ENCODER_ASR32(*unsigned_pointer_14,
                  (uint32_t)signed_value_28 - (uint32_t)signed_value_12));
          DISCARD_SATURATION_STATUS(unsigned_value_31,0x20);
          *unsigned_pointer_17 = unsigned_value_31;
          unsigned_pointer_14 = unsigned_pointer_14 + 1;
          unsigned_pointer_17 = unsigned_pointer_17 + 1;
          unsigned_pointer_21 = unsigned_pointer_22;
        } while (unsigned_pointer_22 != unsigned_pointer_13 + 0x20);
        work_784 = work_784 + 1;
        work_77c = work_77c + 0x16;
        work_770 = work_770 + 1;
        work_780 = work_780 + 0x62;
        work_788 = work_788 + 4;
        work_78c = work_78c + 2;
        unsigned_pointer_14 = work_4c4;
        work_40[3] = signed_value_28;
      } while (work_784 != 2);
      work_760 = work_760 + 0x2c;
      unsigned_value_31 = work_40[0];
      unsigned_value_16 = work_40[1];
      unsigned_pointer_13 = unsigned_pointer_20;
      if ((int)work_40[1] <= (int)work_40[0]) {
        unsigned_value_31 = work_40[1];
        unsigned_value_16 = work_40[0];
        unsigned_pointer_14 = unsigned_pointer_20;
        unsigned_pointer_13 = work_4c4;
      }
      signed_value_28 = 0;
      do {
        unsigned_value_1 = (uint32_t)encoder_saturating_add_i32(
            *(int *)((intptr_t)unsigned_pointer_14 + signed_value_28),
            *(int *)((intptr_t)unsigned_pointer_13 + signed_value_28) >>
                (unsigned_value_16 - unsigned_value_31 & 0xff));
        DISCARD_SATURATION_STATUS(unsigned_value_1,0x20);
        *(uint32_t *)((intptr_t)unsigned_pointer_20 + signed_value_28) = unsigned_value_1;
        signed_value_28 = signed_value_28 + 4;
      } while (signed_value_28 != 0x80);
      work_40[4] = unsigned_value_16;
      *work_764 = unsigned_value_16;
      vocoder_normalize_i32_to_i16(unsigned_pointer_20,unsigned_pointer_20,0x20,work_40 + 4);
      *work_764 = work_40[4];
      work_774 = work_774 + 1;
      work_75c = work_75c + 0xc4;
      work_754 = work_754 + 4;
      unsigned_pointer_20 = unsigned_pointer_20 + 0x10;
      work_764 = work_764 + 1;
    } while (work_774 != 8);
    vocoder_normalize_i32_to_i16(workspace_pointer_504,work_160,0x20,work_40 + 3);
  }
  work_758 = (int)short_value_5;
  signed_value_25 = argument_1 + (0x31 - signed_value_8) * 2;
  unsigned_pointer_29 = workspace_pointer_2c0;
  signed_value_10 = 0;
  do {
    vocoder_copy_i16(signed_value_10 * 0x62 + argument_1,signed_value_8 * 4 + -0x62 + signed_value_25,(int)(short)(0x31 - short_value_5));
    signed_value_10 = signed_value_10 + 1;
    vocoder_copy_i16(signed_value_25,unsigned_pointer_29,work_758);
    signed_value_25 = signed_value_25 + 0x62;
    unsigned_pointer_29 = unsigned_pointer_29 + 0x16;
  } while (signed_value_10 != 0x10);
  if (argument_3 < 1) {
    return;
  }
  unsigned_pointer_20 = work_4c4;
  short_value_5 = vocoder_normalized_windowed_fft_energy
                    (unsigned_pointer_20,argument_4,encoder_window_128,0xfffffff9,0xff,0,unsigned_pointer_20,8,0);
  unsigned_value_9 = vocoder_track_pitch_analysis
                    (argument_2 + 0x614,argument_2 + 0x622,workspace_pointer_504,unsigned_pointer_20,argument_2,
                     (int)*(short *)(argument_2 + 0x91a),(int)argument_7,argument_8,argument_9,
                     (int)*(short *)(argument_6 + 0x180));
  *(uint16_t *)(argument_2 + 0x91e) = *(uint16_t *)(argument_2 + 0x91c);
  signed_value_10 = argument_2 + 0x636;
  *(uint16_t *)(argument_2 + 0x91c) = *(uint16_t *)(argument_2 + 0x91a);
  unsigned_value_6 = vocoder_analyze_spectral_blocks(signed_value_10,unsigned_value_9,work_704,work_60);
  signed_value_8 = (int)*(short *)(argument_2 + 0x622);
  *(uint16_t *)(argument_2 + 0x91a) = unsigned_value_6;
  if (signed_value_8 < 0x375a) {
    if ((0x1000 < signed_value_8) || (*argument_8 != 0)) goto encoder_branch_05;
    short_value_19 = 0;
  }
  else {
    short_value_19 = (short)((uint32_t)(signed_value_8 << 0xf) >> 0x10);
    *(short *)(argument_2 + 0x622) = short_value_19;
    signed_value_8 = (int)short_value_19;
encoder_branch_05:
    vocoder_compare_resampled_bands(signed_value_10,signed_value_8,unsigned_pointer_20);
    short_value_19 = *argument_8;
    if ((short_value_19 != 0) && (*(short *)(argument_2 + 0x644) < 0x1999)) {
      *(uint16_t *)(argument_2 + 0x644) = 0x1999;
      short_value_19 = *argument_8;
    }
  }
  unsigned_pointer_13 = work_160 + 0x1f;
  signed_value_8 = 5;
  unsigned_pointer_14 = unsigned_pointer_13;
  while( true ) {
    wide_value_2 = 0;
    unsigned_pointer_17 = unsigned_pointer_20;
    do {
      unsigned_pointer_21 = unsigned_pointer_17 + 1;
      wide_value_2 = (int)*unsigned_pointer_17 + wide_value_2;
      unsigned_pointer_17 = unsigned_pointer_21;
    } while (unsigned_pointer_21 != unsigned_pointer_20 + (signed_value_8 - 1U & 0xffff) + 1);
    wide_value_2 = (int64_t)(int)((uint32_t)wide_value_2 >> 2 | (int)((uint64_t)wide_value_2 >> 0x20) << 0x1e) * 0x4800;
    unsigned_pointer_14 = unsigned_pointer_14 + 1;
    *unsigned_pointer_14 = (uint32_t)wide_value_2 >> 0xf | (int)((uint64_t)wide_value_2 >> 0x20) << 0x11;
    unsigned_pointer_20 = unsigned_pointer_20 + (signed_value_8 - 1U & 0xffff) + 1;
    if (unsigned_pointer_14 == workspace_pointer_a4) break;
    signed_value_8 = 8;
  }
  wide_value_2 = 0;
  do {
    unsigned_pointer_13 = unsigned_pointer_13 + 1;
    wide_value_2 = (int)*unsigned_pointer_13 + wide_value_2;
    unsigned_value_31 = (uint32_t)wide_value_2;
  } while (unsigned_pointer_13 != workspace_pointer_a4);
  if (wide_value_2 == 0) {
    signed_value_8 = 0;
    short_value_27 = short_value_5;
  }
  else {
    unsigned_value_18 = (uint32_t)(short)((uint64_t)wide_value_2 >> 0x20);
    unsigned_value_16 = unsigned_value_31;
    unsigned_value_30 = unsigned_value_18;
    if ((int)unsigned_value_18 < 0) {
      unsigned_value_16 = ~unsigned_value_31;
      unsigned_value_30 = ~unsigned_value_18;
    }
    signed_value_8 = COUNT_LEADING_ZEROS(unsigned_value_30);
    if (signed_value_8 == 0x20) {
      signed_value_8 = COUNT_LEADING_ZEROS(unsigned_value_16) + 0x20;
    }
    unsigned_value_16 = signed_value_8 - 0x21;
    if ((int)unsigned_value_16 < 0) {
      wide_value_2 = (int64_t)(int)(unsigned_value_18 << (signed_value_8 - 1U & 0xff) | unsigned_value_31 >> (-unsigned_value_16 & 0xff)) * 0xa3;
      signed_value_8 = (int)((uint32_t)wide_value_2 >> 0xf | (int)((uint64_t)wide_value_2 >> 0x20) << 0x11) >> 0x10;
      short_value_27 = short_value_5 + (short)-unsigned_value_16;
    }
    else {
      wide_value_2 = (int64_t)(int)(unsigned_value_31 << (unsigned_value_16 & 0xff)) * 0xa3;
      signed_value_8 = (int)((uint32_t)wide_value_2 >> 0xf | (int)((uint64_t)wide_value_2 >> 0x20) << 0x11) >> 0x10;
      short_value_27 = short_value_5 - (short)unsigned_value_16;
    }
  }
  if (short_value_19 == 0) {
    unsigned_pointer_29 = workspace_pointer_b8;
    short_pointer_11 = (short *)(argument_2 + 0x63e);
    do {
      short_pointer_26 = short_pointer_11 + 1;
      short_value_19 = *short_pointer_11;
      unsigned_value_31 = (uint32_t)(short)(short_value_19 + (short)((int)short_value_19 << 2));
      if (short_value_19 < 0x199a) {
        unsigned_value_30 = *(uint32_t *)(unsigned_pointer_29 + -8);
        unsigned_value_16 = *(uint32_t *)(unsigned_pointer_29 + -4);
        *(uint32_t *)(unsigned_pointer_29 + -8) =
             (uint32_t)((uint64_t)unsigned_value_31 * (uint64_t)unsigned_value_30) >> 0xf |
             ((int)((uint64_t)unsigned_value_31 * (uint64_t)unsigned_value_30 >> 0x20) +
             unsigned_value_30 * ((int)unsigned_value_31 >> 0x1f) + unsigned_value_31 * ((int)unsigned_value_30 >> 0x1f)) * 0x20000;
        *(uint32_t *)(unsigned_pointer_29 + -4) =
             (uint32_t)((uint64_t)unsigned_value_31 * (uint64_t)unsigned_value_16) >> 0xf |
             ((int)((uint64_t)unsigned_value_31 * (uint64_t)unsigned_value_16 >> 0x20) +
             unsigned_value_16 * ((int)unsigned_value_31 >> 0x1f) + unsigned_value_31 * ((int)unsigned_value_16 >> 0x1f)) * 0x20000;
      }
      unsigned_pointer_29 = unsigned_pointer_29 + 8;
      short_pointer_11 = short_pointer_26;
    } while (short_pointer_26 != (short *)(argument_2 + 0x646));
  }
  unsigned_pointer_14 = work_e0 + 7;
  wide_value_2 = 0;
  unsigned_pointer_20 = unsigned_pointer_14;
  do {
    unsigned_pointer_20 = unsigned_pointer_20 + 1;
    wide_value_2 = (int)*unsigned_pointer_20 + wide_value_2;
    unsigned_value_31 = (uint32_t)wide_value_2;
  } while (unsigned_pointer_20 != workspace_pointer_a4);
  if (wide_value_2 == 0) {
    signed_value_25 = 0;
    short_value_19 = short_value_5;
  }
  else {
    unsigned_value_18 = (uint32_t)(short)((uint64_t)wide_value_2 >> 0x20);
    unsigned_value_16 = unsigned_value_31;
    unsigned_value_30 = unsigned_value_18;
    if ((int)unsigned_value_18 < 0) {
      unsigned_value_16 = ~unsigned_value_31;
      unsigned_value_30 = ~unsigned_value_18;
    }
    signed_value_25 = COUNT_LEADING_ZEROS(unsigned_value_30);
    if (signed_value_25 == 0x20) {
      signed_value_25 = COUNT_LEADING_ZEROS(unsigned_value_16) + 0x20;
    }
    unsigned_value_16 = signed_value_25 - 0x21;
    if ((int)unsigned_value_16 < 0) {
      signed_value_25 = (int)(unsigned_value_18 << (signed_value_25 - 1U & 0xff) | unsigned_value_31 >> (-unsigned_value_16 & 0xff)) >> 0x10;
      short_value_19 = short_value_5 + (short)-unsigned_value_16;
    }
    else {
      signed_value_25 = (int)(unsigned_value_31 << (unsigned_value_16 & 0xff)) >> 0x10;
      short_value_19 = short_value_5 - (short)unsigned_value_16;
    }
  }
  short_value_4 = 8;
  signed_value_28 = argument_6;
  do {
    unsigned_pointer_14 = unsigned_pointer_14 + 1;
    unsigned_value_16 = *unsigned_pointer_14;
    unsigned_value_31 = unsigned_value_16;
    if ((int)unsigned_value_16 < 0) {
      unsigned_value_31 = ~unsigned_value_16;
    }
    short_value_7 = (short)unsigned_value_31;
    if (unsigned_value_16 != 0) {
      short_value_7 = (short)COUNT_LEADING_ZEROS(unsigned_value_31) + -1;
    }
    wide_value_2 = (int64_t)(int)(unsigned_value_16 << ((int)short_value_7 & 0xffU)) * 0xa3;
    unsigned_value_6 = vocoder_normalized_add(*(short *)(signed_value_28 + 0x150) * 0x7f5c >> 0xf,
                                (int)*(short *)(signed_value_28 + 0x170),
                                (int)((uint32_t)wide_value_2 >> 0xf | (int)((uint64_t)wide_value_2 >> 0x20) << 0x11)
                                >> 0x10,(int)(short)((short_value_5 + 3) - short_value_7),work_2c);
    short_value_4 = short_value_4 + 1;
    *(uint16_t *)(signed_value_28 + 0x150) = unsigned_value_6;
    *(short *)(signed_value_28 + 0x170) = (short)work_2c[0];
    signed_value_28 = signed_value_28 + 2;
  } while (short_value_4 != 0x10);
  short_value_4 = 0;
  short_value_7 = 8;
  short_pointer_11 = (short *)(argument_6 + 0x14e);
  short_value_5 = 0;
  do {
    short_value_4 = vocoder_normalized_add((int)short_value_4,(int)short_value_5,(int)short_pointer_11[1],(int)short_pointer_11[0x11],work_2c);
    short_value_7 = short_value_7 + 1;
    short_value_5 = (short)work_2c[0];
    short_pointer_11 = short_pointer_11 + 1;
  } while (short_value_7 != 0x10);
  unsigned_value_6 = vocoder_normalized_add(*(short *)(argument_6 + 0x13c) * 0x7f5c >> 0xf,
                              (int)*(short *)(argument_6 + 0x13e),signed_value_8,(int)(short)(short_value_27 + 3),
                              work_2c);
  *(uint16_t *)(argument_6 + 0x13c) = unsigned_value_6;
  signed_value_28 = (int)short_value_4;
  signed_value_12 = (int)short_value_5;
  *(short *)(argument_6 + 0x13e) = (short)work_2c[0];
  *(short *)(argument_6 + 0x182) = (short)(
      (uint16_t)*(short *)(argument_6 + 0x182) << 1);
  signed_value_8 = vocoder_compare_scaled(signed_value_25,(int)(short)(short_value_19 + 3),signed_value_28 * 0x7ae >> 0x10,signed_value_12);
  if (signed_value_8 == 0) {
    unsigned_value_24 = *(uint16_t *)(argument_6 + 0x182);
  }
  else {
    signed_value_8 = vocoder_compare_scaled(signed_value_25,(int)(short)(short_value_19 + 3),2000,0xfffffff0);
    unsigned_value_24 = *(uint16_t *)(argument_6 + 0x182);
    if (signed_value_8 != 0) {
      unsigned_value_24 = unsigned_value_24 | 1;
      *(uint16_t *)(argument_6 + 0x182) = unsigned_value_24;
    }
  }
  if ((unsigned_value_24 & 0xf) == 0) {
    *(uint16_t *)(argument_6 + 0x180) = 0;
  }
  else {
    signed_value_8 = vocoder_compare_scaled(signed_value_28,signed_value_12,0x7a12,0xfffffff6);
    if (signed_value_8 == 0) {
      signed_value_8 = vocoder_compare_scaled(signed_value_28,signed_value_12,0x9c4,0xfffffff3);
      if (signed_value_8 == 0) {
        signed_value_8 = vocoder_compare_scaled(signed_value_28,signed_value_12,0xc35,0xfffffff6);
        if (signed_value_8 == 0) {
          *(uint16_t *)(argument_6 + 0x180) = 0;
          goto encoder_branch_06;
        }
        short_value_27 = 0x4666;
        short_value_5 = *(short *)(argument_6 + 0x13c);
        short_value_19 = *(short *)(argument_6 + 0x13e);
      }
      else {
        short_value_5 = *(short *)(argument_6 + 0x13c);
        short_value_19 = *(short *)(argument_6 + 0x13e);
        short_value_27 = 0x6666;
      }
      signed_value_8 = vocoder_compare_scaled(signed_value_28,signed_value_12,(int)short_value_5 * (int)short_value_27 >> 0xf,(int)short_value_19);
      if (signed_value_8 != 0) {
        *(uint16_t *)(argument_6 + 0x180) = 1;
      }
    }
    else {
      *(uint16_t *)(argument_6 + 0x180) = 1;
    }
  }
encoder_branch_06:
  if (((int)*(short *)(argument_6 + 0x44) & 0x1fU) >> 2 == 0) {
    short_value_5 = 0;
  }
  else {
    short_value_5 = (short)(encoder_popcount8(*(uint8_t *)(argument_2 + 0x91a)) +
                     encoder_popcount8(*(uint8_t *)(argument_2 + 0x91e)));
  }
  if (((*(short *)(argument_2 + 0x62c) < 0x2001) || (*(short *)(argument_2 + 0x630) < 0x2001)) &&
     (3 < (uint32_t)(int)short_value_5)) {
    signed_value_8 = vocoder_compare_scaled_less
                      (*(short *)(argument_6 + 0x48) * 0x550a + -0xb000000 >> 0x10,7,
                       (int)*(short *)(argument_6 + 0x52),(int)*(short *)(argument_6 + 0x72));
    if (((signed_value_8 != 0) &&
        (signed_value_8 = vocoder_compare_scaled_less
                           ((int)*(short *)(argument_6 + 0x56),(int)*(short *)(argument_6 + 0x76),
                            (int)*(short *)(argument_6 + 0x52),(int)*(short *)(argument_6 + 0x72)),
        signed_value_8 != 0)) && (*(short *)(argument_2 + 0x638) < 0x199a)) {
      signed_value_10 = (int)*(short *)(argument_2 + 0x636) * (int)*(short *)(argument_2 + 0x638) * 2 >> 0x10;
      signed_value_8 = signed_value_10 * 0xa000;
      signed_value_10 = signed_value_10 * 0x50000;
      unsigned_value_24 = (uint16_t)((uint32_t)signed_value_10 >> 0x10);
      if (signed_value_8 - (signed_value_10 >> 3) != 0) {
        unsigned_value_24 = (uint16_t)(signed_value_8 >> 0x1f) ^ 0x7fff;
      }
      *(uint16_t *)(argument_2 + 0x636) = unsigned_value_24;
    }
  }
  else {
    signed_value_8 = ((int)short_value_5 << 0x18) >> 0x10;
    vocoder_update_gain_if_above_threshold
              (signed_value_10,(int)*(short *)(argument_6 + 0x122),(int)*(short *)(argument_6 + 0x52),
               (int)*(short *)(argument_6 + 0x72),0x480,0xffffb800,0x42b8,signed_value_8);
    vocoder_update_gain_if_above_threshold
              (argument_2 + 0x638,(int)*(short *)(argument_6 + 0x124),(int)*(short *)(argument_6 + 0x56),
               (int)*(short *)(argument_6 + 0x76),0x400,0xffffb000,0x32b8,signed_value_8);
    vocoder_update_gain_if_above_threshold
              (argument_2 + 0x63a,(int)*(short *)(argument_6 + 0x126),(int)*(short *)(argument_6 + 0x5a),
               (int)*(short *)(argument_6 + 0x7a),0x380,0xffffac00,0x22b8,signed_value_8);
    signed_value_8 = vocoder_compare_scaled_less
                      (*(short *)(argument_6 + 0x48) * 0x550a + -0xd000000 >> 0x10,7,
                       (int)*(short *)(argument_6 + 0x52),(int)*(short *)(argument_6 + 0x72));
    if (((((signed_value_8 != 0) && (*(short *)(argument_6 + 0xfa) < 0x1c00)) &&
         ((signed_value_8 = vocoder_compare_scaled_less
                             ((int)*(short *)(argument_6 + 0x56),(int)*(short *)(argument_6 + 0x76),
                              (int)*(short *)(argument_6 + 0x52),(int)*(short *)(argument_6 + 0x72)),
          signed_value_8 != 0 &&
          (((*(uint16_t *)(argument_2 + 0x91a) & 0x7f) == 0 &&
           ((*(uint16_t *)(argument_2 + 0x91e) & 0x7f) == 0)))))) &&
        (0x1600 < (int)*(short *)(argument_6 + 0x48) - (int)*(short *)(argument_6 + 0x13a))) &&
       (*(short *)(argument_2 + 0x636) < 0x1c29)) {
      *(uint16_t *)(argument_2 + 0x636) = 0x1c29;
    }
  }
  return;
}
