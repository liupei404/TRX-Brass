/* Internal fixed-point spectral-energy implementation. */


int vocoder_update_spectral_energy_state_raw(intptr_t argument_1,int *argument_2,int argument_3,int argument_4)

{
  short *short_pointer_1;
  uint32_t unsigned_value_2;
  short short_value_3;
  short short_value_4;
  short short_value_5;
  uint16_t unsigned_value_6;
  uint16_t unsigned_value_7;
  short short_value_8;
  uint16_t unsigned_value_9;
  uint16_t unsigned_value_10;
  uint32_t unsigned_value_11;
  short *short_pointer_12;
  int signed_value_13;
  short short_value_14;
  short short_value_15;
  uint32_t unsigned_value_16;
  int signed_value_17;
  int signed_value_18;
  uint16_t unsigned_value_19;
  uint32_t unsigned_value_20;
  uint32_t unsigned_value_21;
  uint16_t unsigned_value_22;
  short short_value_23;
  short short_value_24;
  short short_value_25;
  int signed_value_26;
  int signed_value_27;
  int *signed_pointer_28;
  int *signed_pointer_29;
  uint32_t unsigned_value_30;
  short short_value_31;
  int signed_value_32;
  int signed_value_33;
  uint32_t unsigned_value_34;
  bool condition_35;
  short work_e4;
  short work_e0;
  uint32_t work_dc;
  int work_d0;
  uint32_t work_c8;
  uint32_t work_c4;
  uint32_t work_c0;
  int stack_value_bc;
  uint32_t work_b8;
  uint32_t work_b0;
  uint32_t work_ac;
  int work_98;
  short work_88;
  uint16_t *work_7c;
  int work_78 [8];
  int work_58;
  int work_54;
  int work_50;
  uint32_t work_4c;
  uint32_t work_48;
  uint32_t work_44;
  uint32_t work_40;
  uint32_t work_3c;
  int work_38;
  uint32_t work_34;
  uint32_t work_30;
  uint32_t work_2c [2];
  intptr_t state_cursor;
  
  unsigned_value_19 = *(uint16_t *)(argument_1 + 0xfa);
  *(uint16_t *)(argument_1 + 0x100) = *(uint16_t *)(argument_1 + 0xfe);
  *(uint16_t *)(argument_1 + 0xfe) = *(uint16_t *)(argument_1 + 0xfc);
  *(uint16_t *)(argument_1 + 0xfa) = *(uint16_t *)(argument_1 + 0xf8);
  *(uint16_t *)(argument_1 + 0xf8) = *(uint16_t *)(argument_1 + 0xf6);
  *(uint16_t *)(argument_1 + 0xfc) = unsigned_value_19;
  vocoder_copy_i16(argument_1 + 0x122,argument_1 + 0x112,8);
  work_7c = (uint16_t *)(argument_1 + 0x102);
  *(uint16_t *)(argument_1 + 0x136) = *(uint16_t *)(argument_1 + 0x134);
  vocoder_copy_i16(argument_1 + 0x112,work_7c,8);
  signed_value_26 = *argument_2;
  *(uint16_t *)(argument_1 + 0x134) = *(uint16_t *)(argument_1 + 0x132);
  if (signed_value_26 == -0x80000000) {
    unsigned_value_16 = 0x3fffffff;
    signed_value_26 = 0;
  }
  else {
    signed_value_26 = -signed_value_26;
    unsigned_value_16 = signed_value_26 >> 1;
    signed_value_26 = signed_value_26 >> 0x1f;
  }
  signed_value_17 = 0;
  do {
    unsigned_value_11 = *(uint32_t *)((uint8_t *)argument_2 + signed_value_17);
    signed_value_17 = signed_value_17 + 4;
    condition_35 = ADD_HAS_CARRY(unsigned_value_16,unsigned_value_11);
    unsigned_value_16 = unsigned_value_16 + unsigned_value_11;
    signed_value_26 = signed_value_26 + ((int)unsigned_value_11 >> 0x1f) + (uint32_t)condition_35;
  } while (signed_value_17 != 0x40);
  if (unsigned_value_16 == 0 && signed_value_26 == 0) {
    work_58 = argument_3 + 1;
    unsigned_value_16 = 0;
  }
  else {
    unsigned_value_11 = (uint32_t)(short)signed_value_26;
    if ((int)unsigned_value_11 < 0) {
      signed_value_26 = COUNT_LEADING_ZEROS(~unsigned_value_11);
      if (signed_value_26 == 0x20) {
        signed_value_26 = COUNT_LEADING_ZEROS(~unsigned_value_16) + 0x20;
      }
    }
    else {
      signed_value_26 = COUNT_LEADING_ZEROS(unsigned_value_11);
      if (signed_value_26 == 0x20) {
        signed_value_26 = COUNT_LEADING_ZEROS(unsigned_value_16) + 0x20;
      }
    }
    unsigned_value_30 = signed_value_26 - 0x21;
    if ((int)unsigned_value_30 < 0) {
      work_58 = (argument_3 + 1) - unsigned_value_30;
      unsigned_value_16 = unsigned_value_16 >> (-unsigned_value_30 & 0xff) | unsigned_value_11 << (signed_value_26 - 1U & 0xff);
    }
    else {
      work_58 = (argument_3 + 1) - unsigned_value_30;
      unsigned_value_16 = unsigned_value_16 << (unsigned_value_30 & 0xff);
    }
  }
  unsigned_value_30 = argument_4 * 0x10000;
  unsigned_value_11 = unsigned_value_30;
  if ((int)unsigned_value_30 < 0) {
    unsigned_value_11 = ~unsigned_value_30;
  }
  short_value_23 = (short)unsigned_value_11;
  if (unsigned_value_30 != 0) {
    short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
  }
  short_value_23 = vocoder_normalized_divide
                     (unsigned_value_16,work_58,(int)(unsigned_value_30 << ((int)short_value_23 & 0xffU)) >> 0x10,0xf - short_value_23,
                      &work_58);
  if ((short_value_23 == 0) || (work_58 < -0x1f)) {
    unsigned_value_19 = 0xa601;
  }
  else {
    unsigned_value_11 = vocoder_log2_q15((uint16_t)short_value_23,work_58);
    unsigned_value_16 = unsigned_value_11;
    if ((int)unsigned_value_11 < 0) {
      unsigned_value_16 = ~unsigned_value_11;
    }
    short_value_23 = (short)unsigned_value_16;
    if (unsigned_value_11 != 0) {
      short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
    }
    unsigned_value_16 = (uint32_t)short_value_23;
    signed_value_26 = (short)((unsigned_value_11 << (unsigned_value_16 & 0xff)) >> 0x10) * 0xc0a8;
    if ((int)(0xf - unsigned_value_16) < 5) {
      signed_value_26 = signed_value_26 >> (unsigned_value_16 - 10 & 0xff);
    }
    else {
      signed_value_26 = (int32_t)LSL32(signed_value_26,10 - unsigned_value_16);
    }
    unsigned_value_19 = (uint16_t)((uint32_t)(signed_value_26 + 0x4ba7000) >> 0x10);
  }
  *(uint16_t *)(argument_1 + 0x4a) = unsigned_value_19;
  if (*(short *)(argument_1 + 0x48) < -0x1400) {
    signed_value_26 = (short)(*(short *)(argument_1 + 0x48) + 0x1400) * 0x2a85 >> 0xf;
    if (signed_value_26 < -0x352) {
      signed_value_26 = -0x352;
    }
    short_value_23 = vocoder_exp2_scaled(signed_value_26,7,0);
    work_88 = (short)((short_value_23 * 0x459a & 0x7fffffffU) >> 0xf);
  }
  else {
    work_88 = 0x459a;
  }
  unsigned_value_16 = unsigned_value_30;
  if ((int)unsigned_value_30 < 0) {
    unsigned_value_16 = ~unsigned_value_30;
  }
  short_value_23 = (short)unsigned_value_16;
  if (unsigned_value_30 != 0) {
    short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
  }
  short_value_25 = *(short *)(argument_1 + 0x4e);
  work_44 = 0;
  work_58 = 0;
  work_54 = 0;
  work_4c = 0;
  work_48 = 10;
  short_value_23 = vocoder_normalized_divide
                     (0x5b950000,0,(int)(unsigned_value_30 << ((int)short_value_23 & 0xffU)) >> 0x10,0xf - short_value_23,
                      &work_50);
  unsigned_value_16 = 0;
  signed_value_17 = 0;
  short_pointer_12 = (short *)(argument_1 + 0x50);
  signed_value_26 = 0x10;
  do {
    while( true ) {
      signed_value_18 = signed_value_26;
      short_pointer_1 = short_pointer_12 + 1;
      signed_value_26 = (int)short_pointer_12[0x11];
      short_pointer_12 = short_pointer_1;
      if (4 < signed_value_26) break;
      unsigned_value_11 = ASR32(LSL32((int)*short_pointer_1,0x10),5U - signed_value_26);
      condition_35 = ADD_HAS_CARRY(unsigned_value_16,unsigned_value_11);
      unsigned_value_16 = unsigned_value_16 + unsigned_value_11;
      signed_value_17 = signed_value_17 + ((int)unsigned_value_11 >> 0x1f) + (uint32_t)condition_35;
      signed_value_26 = signed_value_18 + -1;
      if (signed_value_18 + -1 == 0) goto analysis_branch_01;
    }
    unsigned_value_11 = LSL32(LSL32((int)*short_pointer_1,0x10),signed_value_26 - 5U);
    condition_35 = ADD_HAS_CARRY(unsigned_value_16,unsigned_value_11);
    unsigned_value_16 = unsigned_value_16 + unsigned_value_11;
    signed_value_17 = signed_value_17 + ((int)unsigned_value_11 >> 0x1f) + (uint32_t)condition_35;
    signed_value_26 = signed_value_18 + -1;
  } while (signed_value_18 + -1 != 0);
analysis_branch_01:
  signed_value_18 = signed_value_18 + -1;
  signed_value_26 = (int32_t)ASR32(unsigned_value_16 >> 4 | LSL32(signed_value_17,0x1c),0x10);
  if (signed_value_26 < -0x5a00) {
    signed_value_26 = -0x5a00;
  }
  state_cursor = argument_1;
  signed_value_27 = signed_value_18;
  signed_pointer_28 = work_78;
  do {
    while( true ) {
      signed_value_13 = (int)*(short *)(state_cursor + 0x72);
      signed_value_33 = (uint32_t)*(uint16_t *)(state_cursor + 0x52) * -0x10000;
      if (4 < signed_value_13) break;
      signed_value_13 = (signed_value_33 >> (5U - signed_value_13 & 0xff)) + signed_value_26 * 0x10000 + 0x28000000 + signed_value_27 * 0x800 >>
               0x10;
      if (0x3ff < signed_value_13) goto analysis_branch_02;
analysis_branch_03:
      signed_value_27 = signed_value_27 + -0x4ccc;
      state_cursor = state_cursor + 2;
      *(short *)signed_pointer_28 = 0x400;
      signed_value_18 = signed_value_18 + 0x400;
      signed_pointer_28 = (int *)((uint8_t *)signed_pointer_28 + 2);
      if (signed_value_27 == -0x4ccc0) goto analysis_branch_04;
    }
    signed_value_13 = (signed_value_33 << (signed_value_13 - 5U & 0xff)) + signed_value_26 * 0x10000 + 0x28000000 + signed_value_27 * 0x800 >>
             0x10;
    if (signed_value_13 < 0x400) goto analysis_branch_03;
analysis_branch_02:
    if (0x27ff < signed_value_13) {
      signed_value_13 = 0x2800;
    }
    signed_value_27 = signed_value_27 + -0x4ccc;
    state_cursor = state_cursor + 2;
    *(short *)signed_pointer_28 = (short)signed_value_13;
    signed_value_18 = signed_value_18 + (short)signed_value_13;
    signed_pointer_28 = (int *)((uint8_t *)signed_pointer_28 + 2);
  } while (signed_value_27 != -0x4ccc0);
analysis_branch_04:
  short_value_3 = vocoder_fixed_divide_half(0x3ffffff,(signed_value_18 << 0xc) >> 0x10);
  for (signed_value_13 = 0; signed_value_13 < 16; signed_value_13 = signed_value_13 + 1) {
    ((short *)work_78)[signed_value_13] = (short)((uint32_t)(
        (int)((short *)work_78)[signed_value_13] * (int)short_value_3 * 2) >> 0x10);
  }
  if (*(short *)(argument_1 + 0x48) < -0x18ff) {
    work_98 = 0x200;
  }
  else {
    work_98 = (short)(*(short *)(argument_1 + 0x48) - *(short *)(argument_1 + 0x50)) * 0x1999 >> 0xf;
    if (0x1ff < work_98) {
      work_98 = 0x200;
    }
    if (work_98 < 0x100) {
      work_98 = 0x100;
    }
  }
  work_c0 = 0;
  stack_value_bc = 0;
  signed_value_17 = 0;
  work_d0 = 0;
  work_ac = 0;
  unsigned_value_9 = 0;
  short_value_8 = 0;
  work_c4 = 0;
  work_b0 = 0;
  work_b8 = 0;
  short_value_3 = 32000;
  unsigned_value_10 = 0;
  unsigned_value_16 = 0;
  state_cursor = argument_1;
  unsigned_value_22 = 0;
  do {
    unsigned_value_20 = *(uint32_t *)(argument_1 + signed_value_17);
    unsigned_value_30 = *(uint32_t *)((uint8_t *)argument_2 + signed_value_17);
    unsigned_value_11 = unsigned_value_30;
    if ((int)unsigned_value_30 < 0) {
      unsigned_value_11 = ~unsigned_value_30;
    }
    short_value_14 = (short)unsigned_value_11;
    if (unsigned_value_30 != 0) {
      short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
    }
    unsigned_value_11 = unsigned_value_20;
    if ((int)unsigned_value_20 < 0) {
      unsigned_value_11 = ~unsigned_value_20;
    }
    short_value_24 = (short)unsigned_value_11;
    if (unsigned_value_20 != 0) {
      short_value_24 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
    }
    short_value_4 = vocoder_normalized_add((int)(unsigned_value_30 << ((int)short_value_14 & 0xffU)) >> 0x10,argument_3 - short_value_14,
                                (int)(unsigned_value_20 << ((int)short_value_24 & 0xffU)) >> 0x10,
                                (int)*(short *)(argument_1 + 0xf2) - (int)short_value_24,&work_38);
    work_e4 = *(short *)(state_cursor + 0x52);
    short_value_14 = *(short *)(state_cursor + 0x92);
    short_value_24 = *(short *)(state_cursor + 0xb2);
    unsigned_value_30 = (uint32_t)*(short *)(state_cursor + 0x72);
    unsigned_value_34 = (int)short_value_4 * (int)short_value_23 * 2;
    unsigned_value_20 = work_d0 * 0x800 + 0x40000000;
    work_38 = work_38 + -1;
    unsigned_value_11 = unsigned_value_34;
    if ((int)unsigned_value_34 < 0) {
      unsigned_value_11 = ~unsigned_value_34;
    }
    short_value_15 = (short)unsigned_value_11;
    if (unsigned_value_34 != 0) {
      short_value_15 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
    }
    unsigned_value_11 = unsigned_value_20;
    if ((int)unsigned_value_20 < 0) {
      unsigned_value_11 = ~unsigned_value_20;
    }
    short_value_31 = (short)unsigned_value_11;
    if (unsigned_value_20 != 0) {
      short_value_31 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
    }
    short_value_15 = vocoder_normalized_add((int)(unsigned_value_34 << ((int)short_value_15 & 0xffU)) >> 0x10,
                                 (work_38 + work_50) - (int)short_value_15,
                                 (int)(unsigned_value_20 << ((int)short_value_31 & 0xffU)) >> 0x10,-0x1e - short_value_31,
                                 &work_40);
    signed_value_18 = (int)short_value_4;
    if (short_value_15 == 0) {
      work_40 = 0xf;
      signed_value_27 = -0x8000;
      work_c8 = 0x8000;
    }
    else {
      unsigned_value_20 = vocoder_log2_q15((int)short_value_15,work_40);
      unsigned_value_11 = unsigned_value_20;
      if ((int)unsigned_value_20 < 0) {
        unsigned_value_11 = ~unsigned_value_20;
      }
      short_value_15 = (short)unsigned_value_11;
      if (unsigned_value_20 != 0) {
        short_value_15 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
      }
      unsigned_value_20 = unsigned_value_20 << ((int)short_value_15 & 0xffU);
      work_c8 = unsigned_value_20 >> 0x10;
      work_40 = 0xf - (int)short_value_15;
      signed_value_27 = (int)(short)(unsigned_value_20 >> 0x10);
    }
    signed_value_13 = (int)work_e4;
    short_value_5 = vocoder_normalized_add(signed_value_13,unsigned_value_30,work_98,7,&work_3c);
    short_value_15 = *(short *)(argument_1 + 0x138);
    short_value_31 = short_value_25;
    if (short_value_15 < 8) {
      unsigned_value_20 = (int)work_e4 * (int)short_value_15 * 2;
      unsigned_value_11 = unsigned_value_20;
      if ((int)unsigned_value_20 < 0) {
        unsigned_value_11 = ~unsigned_value_20;
      }
      short_value_14 = (short)unsigned_value_11;
      if (unsigned_value_20 != 0) {
        short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
      }
      work_34 = (unsigned_value_30 + 0xf) - (int)short_value_14;
      signed_value_13 = vocoder_normalized_add((int)(unsigned_value_20 << ((int)short_value_14 & 0xffU)) >> 0x10,work_34,signed_value_27,
                                   work_40,&work_34);
      unsigned_value_30 = short_value_15 * 0x10000 + 0x10000;
      unsigned_value_11 = unsigned_value_30;
      if ((int)unsigned_value_30 < 0) {
        unsigned_value_11 = ~unsigned_value_30;
      }
      short_value_14 = (short)unsigned_value_11;
      if (unsigned_value_30 != 0) {
        short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_11) + -1;
      }
      work_e4 = vocoder_normalized_divide
                           ((int32_t)LSL32(signed_value_13,0x10),work_34,(int)(unsigned_value_30 << ((int)short_value_14 & 0xffU)) >> 0x10,
                            0xf - short_value_14,&work_34);
analysis_branch_05:
      signed_value_13 = (int)work_e4;
      unsigned_value_19 = 5;
      work_e0 = 0x7fff;
      unsigned_value_30 = work_34;
    }
    else {
      signed_value_33 = vocoder_compare_scaled_less(signed_value_27,work_40,(int)short_value_5,work_3c);
      unsigned_value_11 = work_40;
      if (signed_value_33 != 0) {
        work_34 = unsigned_value_30;
        signed_value_33 = vocoder_compare_scaled_less(signed_value_27,work_40,signed_value_13,unsigned_value_30);
        if ((signed_value_33 == 0) ||
           (signed_value_33 = vocoder_compare_scaled_less(
                signed_value_27,unsigned_value_11,(int16_t)-0x7000,5), signed_value_33 == 0)) {
          signed_value_33 = 0x7333;
          signed_value_32 = 0x6666;
          unsigned_value_11 = unsigned_value_11 - 3;
        }
        else {
          signed_value_33 = 0x4000;
          signed_value_32 = 0x4000;
        }
        unsigned_value_20 = signed_value_32 * signed_value_27 * 2;
        unsigned_value_34 = signed_value_33 * signed_value_13 * 2;
        unsigned_value_30 = unsigned_value_20;
        if ((int)unsigned_value_20 < 0) {
          unsigned_value_30 = ~unsigned_value_20;
        }
        short_value_14 = (short)unsigned_value_30;
        if (unsigned_value_20 != 0) {
          short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_30) + -1;
        }
        unsigned_value_30 = unsigned_value_34;
        if ((int)unsigned_value_34 < 0) {
          unsigned_value_30 = ~unsigned_value_34;
        }
        short_value_24 = (short)unsigned_value_30;
        if (unsigned_value_34 != 0) {
          short_value_24 = (short)COUNT_LEADING_ZEROS(unsigned_value_30) + -1;
        }
        work_e4 = vocoder_normalized_add((int)(unsigned_value_20 << ((int)short_value_14 & 0xffU)) >> 0x10,
                                       unsigned_value_11 - (int)short_value_14,
                                       (int)(unsigned_value_34 << ((int)short_value_24 & 0xffU)) >> 0x10,
                                       work_34 - (int)short_value_24,&work_34);
        goto analysis_branch_05;
      }
      short_value_15 = *(short *)(state_cursor + 0xd2);
      unsigned_value_20 = (uint32_t)short_value_24;
      if (short_value_15 < 1) {
        signed_value_13 = (int)short_value_14;
        work_e0 = 0x7fff;
        unsigned_value_19 = 5;
        unsigned_value_30 = unsigned_value_20;
        work_e4 = short_value_14;
      }
      else {
        work_34 = unsigned_value_20;
        short_value_24 = vocoder_normalized_subtract(0x4000,2,(int)short_value_14,unsigned_value_20,&work_30);
        short_value_31 = vocoder_normalized_add(work_98,7,(int)short_value_14,work_34,work_2c);
        signed_value_33 = vocoder_compare_scaled_less(signed_value_27,unsigned_value_11,(int)short_value_24,work_30);
        if (signed_value_33 == 0) {
          signed_value_33 = vocoder_compare_scaled_less(signed_value_27,unsigned_value_11,(int)short_value_31,work_2c[0]);
          work_e0 = short_value_14;
          if (signed_value_33 != 0) {
            unsigned_value_34 = signed_value_27 * 0xcccc;
            unsigned_value_21 = short_value_14 * 0xe666;
            unsigned_value_20 = unsigned_value_21;
            if ((int)unsigned_value_21 < 0) {
              unsigned_value_20 = ~unsigned_value_21;
            }
            short_value_14 = (short)unsigned_value_20;
            if (unsigned_value_21 != 0) {
              short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_20) + -1;
            }
            unsigned_value_20 = unsigned_value_34;
            if ((int)unsigned_value_34 < 0) {
              unsigned_value_20 = ~unsigned_value_34;
            }
            short_value_24 = (short)unsigned_value_20;
            if (unsigned_value_34 != 0) {
              short_value_24 = (short)COUNT_LEADING_ZEROS(unsigned_value_20) + -1;
            }
            work_30 = work_34 - (int)short_value_14;
            work_2c[0] = (unsigned_value_11 - 3) - (int)short_value_24;
            work_e0 = vocoder_normalized_add((int)(unsigned_value_21 << ((int)short_value_14 & 0xffU)) >> 0x10,work_30,
                                           (int)(unsigned_value_34 << ((int)short_value_24 & 0xffU)) >> 0x10,
                                           work_2c[0],&work_34);
          }
        }
        else {
          work_34 = unsigned_value_11;
          work_e0 = (short)work_c8;
        }
        short_value_31 = short_value_15 + -1;
        unsigned_value_19 = (uint16_t)work_34;
      }
    }
    signed_value_33 = work_38;
    unsigned_value_11 = work_40;
    *(short *)(state_cursor + 0x92) = work_e0;
    *(short *)(state_cursor + 0xd2) = short_value_31;
    *(short *)(state_cursor + 0x52) = work_e4;
    *(uint16_t *)(state_cursor + 0xb2) = unsigned_value_19;
    *(short *)(state_cursor + 0x72) = (short)unsigned_value_30;
    *(uint32_t *)(argument_1 + signed_value_17) =
        *(uint32_t *)((uint8_t *)argument_2 + signed_value_17);
    unsigned_value_6 = vocoder_normalized_subtract(signed_value_13,unsigned_value_30,signed_value_27,work_40,work_2c);
    unsigned_value_20 = work_2c[0];
    signed_value_32 = (int)(short)unsigned_value_6;
    unsigned_value_7 = vocoder_normalized_subtract(signed_value_27,unsigned_value_11,signed_value_13,unsigned_value_30,&work_30);
    unsigned_value_11 = work_30;
    if (-1 < signed_value_32) {
      unsigned_value_7 = unsigned_value_6;
      unsigned_value_11 = work_2c[0];
    }
    if ((int)unsigned_value_16 < 6) {
      unsigned_value_21 = (int)short_value_4 * (int)(short)unsigned_value_7 * 2;
      unsigned_value_34 = unsigned_value_21;
      if ((int)unsigned_value_21 < 0) {
        unsigned_value_34 = ~unsigned_value_21;
      }
      short_value_14 = (short)unsigned_value_34;
      if (unsigned_value_21 != 0) {
        short_value_14 = (short)COUNT_LEADING_ZEROS(unsigned_value_34) + -1;
      }
      work_30 = (unsigned_value_11 + signed_value_33) - (int)short_value_14;
      unsigned_value_10 = vocoder_normalized_add((int)(short)unsigned_value_10,work_4c,
                                   (int)(unsigned_value_21 << ((int)short_value_14 & 0xffU)) >> 0x10,work_30,&work_4c
                                  );
      short_value_3 = vocoder_normalized_add((int)short_value_3,work_48,signed_value_18,signed_value_33,&work_48);
    }
    signed_value_33 = work_38;
    unsigned_value_11 = work_40;
    short_value_14 = *(short *)(argument_1 + 0x48);
    short_value_24 = *(short *)(argument_1 + 0x50);
    short_value_4 = *(short *)((uint8_t *)work_78 + unsigned_value_16 * 2);
    short_value_15 = (short)work_ac;
    if (work_c8 == 0) {
      signed_value_18 = (int)short_value_15;
      signed_value_13 = (int)(short)work_b8;
      short_value_14 = vocoder_normalized_subtract(signed_value_18,work_b0,signed_value_13,work_c4,work_2c);
      if (short_value_14 < 1) {
        work_c4 = work_c8;
        work_b8 = work_c4;
        goto analysis_branch_06;
      }
      work_b0 = work_c4;
      work_c4 = work_c8;
      work_ac = work_b8;
      work_b8 = work_c8;
      if ((unsigned_value_16 & 1) == 0) goto analysis_branch_07;
analysis_branch_10:
      unsigned_value_6 = vocoder_normalized_add((int)(short)unsigned_value_22,work_44,signed_value_32,unsigned_value_20,&work_44);
      if ((int)work_44 < 7) {
        unsigned_value_19 = (uint16_t)((uint32_t)((int)((uint32_t)unsigned_value_6 << 0x10) >> (7 - work_44 & 0xff)) >> 0x10);
      }
      else {
        unsigned_value_19 = (uint16_t)((((uint32_t)unsigned_value_6 << 0x10) << (work_44 - 7 & 0xff)) >> 0x10);
      }
      *work_7c = unsigned_value_19;
      work_7c = work_7c + 1;
    }
    else {
      unsigned_value_21 = ((0x10000 - ((int)(unsigned_value_16 << 0x10) >> 5)) * 0x4000 >> 0x10) * (int)work_88 * 2;
      unsigned_value_34 = unsigned_value_21;
      if ((int)unsigned_value_21 < 0) {
        unsigned_value_34 = ~unsigned_value_21;
      }
      short_value_31 = (short)unsigned_value_34;
      if (unsigned_value_21 != 0) {
        short_value_31 = (short)COUNT_LEADING_ZEROS(unsigned_value_34) + -1;
      }
      short_value_31 = vocoder_normalized_add(signed_value_18,work_38,(int)(unsigned_value_21 << ((int)short_value_31 & 0xffU)) >> 0x10,
                                   -0x13 - short_value_31,&work_30);
      short_value_31 = vocoder_normalized_divide(signed_value_18 << 0x10,signed_value_33,(int)short_value_31,work_30,&work_34);
      signed_value_18 = (int)short_value_4 * (int)short_value_31 * 2 >> 0x10;
      work_34 = work_34 + 5;
      short_value_8 = vocoder_normalized_add((int)short_value_8,work_54,signed_value_18,work_34,&work_54);
      unsigned_value_7 = vocoder_normalized_subtract(signed_value_27,unsigned_value_11,signed_value_13,unsigned_value_30,work_2c);
      work_dc = (uint32_t)unsigned_value_7;
      unsigned_value_11 = (uint32_t)(short)unsigned_value_7;
      if ((int)unsigned_value_11 < 0) {
        if (unsigned_value_11 == 0xffff8000) {
          unsigned_value_11 = 0x7fff;
          work_dc = 0x7fff;
        }
        else {
          unsigned_value_11 = -work_dc;
          work_dc = unsigned_value_11 & 0xffff;
          unsigned_value_11 = (uint32_t)(short)unsigned_value_11;
        }
      }
      unsigned_value_34 = unsigned_value_11 * signed_value_18 * 2;
      unsigned_value_30 = unsigned_value_34;
      if ((int)unsigned_value_34 < 0) {
        unsigned_value_30 = ~unsigned_value_34;
      }
      short_value_4 = (short)unsigned_value_30;
      if (unsigned_value_34 != 0) {
        short_value_4 = (short)COUNT_LEADING_ZEROS(unsigned_value_30) + -1;
      }
      work_34 = (work_34 + work_2c[0]) - (int)short_value_4;
      unsigned_value_9 = vocoder_normalized_add((int)(short)unsigned_value_9,work_58,
                                  (int)(unsigned_value_34 << ((int)short_value_4 & 0xffU)) >> 0x10,work_34,&work_58);
      work_34 = work_2c[0];
      if ((0x800 < (int)short_value_14 - (int)short_value_24) || (short_value_14 < -0x1900)) {
        if (unsigned_value_16 == 0) {
          work_dc = (unsigned_value_11 & 0x1ffff) >> 1;
          unsigned_value_11 = (uint32_t)(short)((unsigned_value_11 << 0xf) >> 0x10);
        }
        else if (unsigned_value_16 == 1) {
          work_dc = (unsigned_value_11 * 3 & 0x3ffff) >> 2;
          unsigned_value_11 = (uint32_t)(short)(unsigned_value_11 * 0xc000 >> 0x10);
        }
      }
      unsigned_value_7 = vocoder_normalized_add((int)(short)work_b8,work_c4,unsigned_value_11,work_2c[0],&work_30);
      short_value_14 = vocoder_normalized_subtract((int)short_value_15,work_b0,(int)(short)unsigned_value_7,work_30,work_2c);
      if (0 < short_value_14) {
        work_ac = (uint32_t)unsigned_value_7;
        work_b0 = work_30;
      }
      work_c4 = work_34;
      signed_value_18 = (int)(short)unsigned_value_7;
      if (short_value_14 < 1) {
        signed_value_18 = (int)short_value_15;
      }
      work_b8 = work_dc;
analysis_branch_06:
      signed_value_13 = signed_value_18;
      if ((unsigned_value_16 & 1) != 0) goto analysis_branch_10;
analysis_branch_07:
      work_44 = unsigned_value_20;
      if (unsigned_value_16 == 0) {
        if ((int)unsigned_value_20 < 5) {
          unsigned_value_19 = (uint16_t)(ASR32(LSL32(signed_value_32,0x10),5 - unsigned_value_20) >> 0x10);
        }
        else {
          unsigned_value_19 = (uint16_t)(LSL32(LSL32(signed_value_32,0x10),unsigned_value_20 - 5) >> 0x10);
        }
        *(uint16_t *)(argument_1 + 0x132) = unsigned_value_19;
      }
    }
    if ((int)work_40 < 7) {
      unsigned_value_11 = ASR32(LSL32(signed_value_27,0x10),7 - work_40);
    }
    else {
      unsigned_value_11 = LSL32(LSL32(signed_value_27,0x10),work_40 - 7);
    }
    signed_value_17 = signed_value_17 + 4;
    condition_35 = ADD_HAS_CARRY(work_c0,unsigned_value_11);
    work_c0 = work_c0 + unsigned_value_11;
    stack_value_bc = stack_value_bc + ((int)unsigned_value_11 >> 0x1f) + (uint32_t)condition_35;
    state_cursor = state_cursor + 2;
    work_d0 = work_d0 + -0x4000;
    unsigned_value_16 = unsigned_value_16 + 1;
    unsigned_value_22 = unsigned_value_6;
  } while (signed_value_17 != 0x40);
  *(short *)(argument_1 + 0x13a) =
       (short)((uint32_t)((*(short *)(argument_1 + 0x13a) * 0x6666 +
                      (short)((work_c0 >> 4 | stack_value_bc * 0x10000000) + 0x7000000 >> 0x10) * 0x4d0e
                      ) * 2) >> 0x10);
  signed_value_26 = vocoder_normalized_divide((uint32_t)unsigned_value_10 << 0x10,work_4c,(int)short_value_3,work_48,&work_40);
  signed_value_26 = (int32_t)((uint32_t)signed_value_26 * 0x10000U);
  if ((int)work_40 < 3) {
    unsigned_value_22 = (uint16_t)(ASR32(signed_value_26,2 - work_40) >> 0x10);
  }
  else {
    signed_value_17 = (int32_t)LSL32(signed_value_26,work_40 - 2);
    unsigned_value_22 = (uint16_t)((uint32_t)signed_value_17 >> 0x10);
    if ((uint32_t)signed_value_26 != ASR32(signed_value_17,work_40 - 2)) {
      unsigned_value_22 = (uint16_t)(signed_value_26 >> 0x1f) ^ 0x7fff;
    }
  }
  *(uint16_t *)(argument_1 + 0xf4) = unsigned_value_22;
  signed_value_13 = signed_value_13 * 0x3332;
  if ((int)work_b0 < 4) {
    unsigned_value_16 = ASR32(signed_value_13,3 - work_b0);
  }
  else {
    unsigned_value_16 = LSL32(signed_value_13,work_b0 - 3);
    condition_35 = (uint32_t)signed_value_13 != ASR32(unsigned_value_16,work_b0 - 3);
    if (condition_35) {
      unsigned_value_16 = 0x7fffffff;
    }
    if (condition_35) {
      unsigned_value_16 = unsigned_value_16 ^ signed_value_13 >> 0x1f;
    }
  }
  unsigned_value_2 = (uint32_t)analysis_saturating_add_i32(
      (int32_t)unsigned_value_16,0x8000);
  DISCARD_SATURATION_STATUS(unsigned_value_2,0x20);
  if (work_54 < 3) {
    signed_value_26 = 0x4000;
    signed_value_17 = 3;
  }
  else {
    signed_value_26 = (int)short_value_8;
    signed_value_17 = work_54;
  }
  signed_value_26 = vocoder_normalized_divide((uint32_t)unsigned_value_9 << 0x10,work_58,signed_value_26,signed_value_17,&work_58);
  signed_value_26 = (int32_t)LSL32(signed_value_26,16);
  if (work_58 < 4) {
    unsigned_value_16 = ASR32(signed_value_26,3U - work_58);
  }
  else {
    /* ARM register shifts use unsigned bit patterns and the low byte of
     * the shift count. Compare the restored value without signed overflow. */
    unsigned_value_16 = LSL32(signed_value_26,work_58 - 3U);
    condition_35 = (uint32_t)signed_value_26 != ASR32(unsigned_value_16,work_58 - 3U);
    if (condition_35) {
      unsigned_value_16 = 0x7fffffff;
    }
    if (condition_35) {
      unsigned_value_16 = unsigned_value_16 ^ signed_value_26 >> 0x1f;
    }
  }
  signed_value_26 = analysis_saturating_add_i32((int32_t)unsigned_value_16,0x8000);
  DISCARD_SATURATION_STATUS(signed_value_26,0x20);
  signed_value_26 = signed_value_26 >> 0x10;
  short_value_25 = *(short *)(argument_1 + 0x4a);
  *(short *)(argument_1 + 0xf2) = (short)argument_3;
  short_value_23 = *(short *)(argument_1 + 0x48);
  signed_value_17 = (int)short_value_25;
  if ((signed_value_26 < 0x4001) || (signed_value_17 < -0x27ff)) {
    signed_value_18 = (int)short_value_23;
    if ((signed_value_18 < signed_value_17) || (0x2000 < signed_value_26)) goto analysis_branch_08;
    if (-0x2800 < signed_value_18) {
      unsigned_value_30 = signed_value_18 * 0xffbe;
      unsigned_value_11 = short_value_25 * 0x8312;
      unsigned_value_16 = unsigned_value_11;
      if ((int)unsigned_value_11 < 0) {
        unsigned_value_16 = ~unsigned_value_11;
      }
      short_value_23 = (short)unsigned_value_16;
      if (unsigned_value_11 != 0) {
        short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      unsigned_value_16 = unsigned_value_30;
      if ((int)unsigned_value_30 < 0) {
        unsigned_value_16 = ~unsigned_value_30;
      }
      short_value_25 = (short)unsigned_value_16;
      if (unsigned_value_30 != 0) {
        short_value_25 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      signed_value_17 = vocoder_normalized_add((int)(unsigned_value_11 << ((int)short_value_23 & 0xffU)) >> 0x10,-2 - short_value_23,
                                   (int)(unsigned_value_30 << ((int)short_value_25 & 0xffU)) >> 0x10,7 - short_value_25,
                                   &work_58);
      goto analysis_joined_branch;
    }
  }
  else {
    if (short_value_23 < signed_value_17) {
      unsigned_value_11 = short_value_25 * 0xcccc;
      unsigned_value_30 = short_value_23 * 0xe666;
      unsigned_value_16 = unsigned_value_11;
      if ((int)unsigned_value_11 < 0) {
        unsigned_value_16 = ~unsigned_value_11;
      }
      short_value_23 = (short)unsigned_value_16;
      if (unsigned_value_11 != 0) {
        short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      unsigned_value_16 = unsigned_value_30;
      if ((int)unsigned_value_30 < 0) {
        unsigned_value_16 = ~unsigned_value_30;
      }
      short_value_25 = (short)unsigned_value_16;
      if (unsigned_value_30 != 0) {
        short_value_25 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      unsigned_value_16 = (uint32_t)short_value_25;
      signed_value_17 = unsigned_value_11 << ((int)short_value_23 & 0xffU);
      signed_value_27 = unsigned_value_30 << (unsigned_value_16 & 0xff);
      signed_value_18 = 4 - short_value_23;
    }
    else {
analysis_branch_08:
      unsigned_value_11 = short_value_25 * 0xa3d8;
      unsigned_value_30 = short_value_23 * 0xfd70;
      unsigned_value_16 = unsigned_value_11;
      if ((int)unsigned_value_11 < 0) {
        unsigned_value_16 = ~unsigned_value_11;
      }
      short_value_23 = (short)unsigned_value_16;
      if (unsigned_value_11 != 0) {
        short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      unsigned_value_16 = unsigned_value_30;
      if ((int)unsigned_value_30 < 0) {
        unsigned_value_16 = ~unsigned_value_30;
      }
      short_value_25 = (short)unsigned_value_16;
      if (unsigned_value_30 != 0) {
        short_value_25 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
      }
      unsigned_value_16 = (uint32_t)short_value_25;
      signed_value_17 = unsigned_value_11 << ((int)short_value_23 & 0xffU);
      signed_value_27 = unsigned_value_30 << (unsigned_value_16 & 0xff);
      signed_value_18 = 1 - short_value_23;
    }
    signed_value_17 = vocoder_normalized_add(signed_value_17 >> 0x10,signed_value_18,signed_value_27 >> 0x10,7 - unsigned_value_16,&work_58);
analysis_joined_branch:
    if (work_58 < 7) {
      short_value_23 = (short)(ASR32(LSL32(signed_value_17,0x10),7U - work_58) >> 0x10);
    }
    else {
      short_value_23 = (short)(LSL32(LSL32(signed_value_17,0x10),work_58 - 7U) >> 0x10);
    }
    short_value_25 = *(short *)(argument_1 + 0x4a);
    signed_value_17 = (int)short_value_25;
  }
  *(short *)(argument_1 + 0x48) = short_value_23;
  signed_value_18 = (int)(short)((uint32_t)unsigned_value_2 >> 0x10);
  if (signed_value_26 < signed_value_18) {
    signed_value_26 = signed_value_18;
  }
  signed_value_18 = *(int8_t *)(argument_1 + 0x4c) * 0x100;
  signed_value_26 = signed_value_26 * 0x10000;
  short_value_3 = *(short *)(argument_1 + 0x138);
  if ((-0x5a00 < signed_value_18) && (signed_value_17 <= signed_value_18)) {
    if (short_value_3 < 8) {
      signed_value_26 = signed_value_26 + -0x38000000;
    }
    else {
      signed_value_17 = signed_value_17 * 0x10000 + short_value_23 * -0x10000;
      if (signed_value_17 < -0x13fffaff) {
        if (signed_value_17 < -0x13ffffff) {
          if (signed_value_17 < -0x1dffffff) {
            if (signed_value_17 < -0x27ffffff) {
              if (-0x32000000 < signed_value_17) {
                unsigned_value_16 = (uint32_t)(short)*(uint16_t *)(argument_1 + 0x44);
                signed_value_18 = 0x12;
                signed_value_27 = 0;
                do {
                  unsigned_value_11 = unsigned_value_16 & 1;
                  unsigned_value_16 = (int)unsigned_value_16 >> 1;
                  signed_value_18 = signed_value_18 + -1;
                  signed_value_27 = signed_value_27 + unsigned_value_11;
                } while (signed_value_18 != 0);
                if (1 < signed_value_27) {
                  signed_value_26 = signed_value_26 - ((short)((uint32_t)(signed_value_17 + 0x32000000) >> 0x10) * -0x33328 +
                                    0x3c000000);
                  goto analysis_branch_09;
                }
              }
              signed_value_26 = signed_value_26 + -0x1c000000;
            }
            else {
              signed_value_26 = signed_value_26 - (((short)((uint32_t)(signed_value_17 + 0x28000000) >> 0x10) * -0xcccc >> 1) +
                                0x1c000000);
            }
          }
          else {
            signed_value_26 = signed_value_26 - ((short)((uint32_t)(signed_value_17 + 0x1e000000) >> 0x10) * -0xccca + 0x18000000);
          }
          goto analysis_branch_09;
        }
      }
      else if (((int)((uint32_t)*(uint16_t *)(argument_1 + 0x44) << 0x1f) < 0) ||
              ((int)short_value_23 - (int)*(short *)(argument_1 + 0x50) < 0x800)) {
        signed_value_26 = signed_value_26 + -0x9998000;
        goto analysis_branch_09;
      }
      signed_value_26 = signed_value_26 + -0x10000000;
    }
  }
analysis_branch_09:
  unsigned_value_19 = (uint16_t)((uint32_t)signed_value_26 >> 0x10);
  if (signed_value_26 < 0x10000) {
    unsigned_value_30 = short_value_25 * 0xcccc;
    unsigned_value_11 = *(short *)(argument_1 + 0x50) * 0xe666;
    unsigned_value_16 = unsigned_value_30;
    if ((int)unsigned_value_30 < 0) {
      unsigned_value_16 = ~unsigned_value_30;
    }
    short_value_23 = (short)unsigned_value_16;
    if (unsigned_value_30 != 0) {
      short_value_23 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
    }
    unsigned_value_16 = unsigned_value_11;
    if ((int)unsigned_value_11 < 0) {
      unsigned_value_16 = ~unsigned_value_11;
    }
    short_value_25 = (short)unsigned_value_16;
    if (unsigned_value_11 != 0) {
      short_value_25 = (short)COUNT_LEADING_ZEROS(unsigned_value_16) + -1;
    }
    *(uint16_t *)(argument_1 + 0xf6) = unsigned_value_19;
    signed_value_26 = vocoder_normalized_add((int)(unsigned_value_30 << ((int)short_value_23 & 0xffU)) >> 0x10,4 - short_value_23,
                                 (int)(unsigned_value_11 << ((int)short_value_25 & 0xffU)) >> 0x10,7 - short_value_25,&work_58
                                );
    if (work_58 < 7) {
      unsigned_value_19 = (uint16_t)(ASR32(LSL32(signed_value_26,0x10),7U - work_58) >> 0x10);
    }
    else {
      unsigned_value_19 = (uint16_t)(LSL32(LSL32(signed_value_26,0x10),work_58 - 7U) >> 0x10);
    }
    *(uint16_t *)(argument_1 + 0x50) = unsigned_value_19;
    short_value_3 = *(short *)(argument_1 + 0x138);
    unsigned_value_22 = 0;
  }
  else {
    *(uint16_t *)(argument_1 + 0xf6) = unsigned_value_19;
    unsigned_value_22 = 1;
  }
  if (short_value_3 < 8) {
    *(short *)(argument_1 + 0x138) = short_value_3 + 1;
  }
  unsigned_value_22 = unsigned_value_22 | LSL32(*(short *)(argument_1 + 0x44),1);
  *(uint16_t *)(argument_1 + 0x44) = unsigned_value_22;
  return (int)(short)unsigned_value_22;
}
