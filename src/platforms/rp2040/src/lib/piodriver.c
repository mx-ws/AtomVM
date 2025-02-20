/*
 * This file is part of AtomVM.
 *
 * Copyright 2023 Paul Guyot <pguyot@kallisys.net>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0 OR LGPL-2.1-or-later
 */

#include "piodriver.h"

#include <stdbool.h>
#include <string.h>


#ifdef LIB_PICO_CYW43_ARCH
#include <pico/cyw43_arch.h>
#endif

#include "defaultatoms.h"
#include "interop.h"
#include "rp2040_sys.h"
#include "trace.h"


#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"

#define WL_ATOM globalcontext_make_atom(ctx->global, ATOM_STR("\x2", "wl"))

// by default flash leds on gpios 3-4
#ifndef PIO_BLINK_LED1_GPIO
#define PIO_BLINK_LED1_GPIO 25
#endif
#define PROGRAM_LENGTH 8

static const struct Nif *pio_nif_get_nif(const char *nifname);

void blink_pin_forever(PIO pio, uint sm, uint offset, uint pin, uint freq) {
    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_config c = pio_get_default_sm_config();

    sm_config_set_set_pins(&c, pin, 1);
    sm_config_set_clkdiv(&c, 0.01f);
    sm_config_set_wrap(&c, offset, offset + PROGRAM_LENGTH);

    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);

    //printf("Blinking pin %d at %d Hz\n", pin, freq);

    // PIO counter program takes 3 more cycles in total than we pass as
    // input (wait for n + 1; mov; jmp)
    //pio->txf[sm] = (clock_get_hz(clk_sys) / (2 * freq)) - 3;
}

int pio_helper() {
    setup_default_uart();

    assert(PIO_BLINK_LED1_GPIO < 31);
    assert(PIO_BLINK_LED3_GPIO < 31 || PIO_BLINK_LED3_GPIO >= 32);

    PIO pio = pio0;
    uint sm;
    uint offset;

    uint16_t my_blink_prog_instr[] = 
        { pio_encode_set(pio_pins, 1) | pio_encode_delay(29)
        , pio_encode_set(pio_x, 31)
        , pio_encode_nop() | pio_encode_delay(29)
        , pio_encode_jmp_x_dec(2)
        , pio_encode_set(pio_pins, 0) | pio_encode_delay(29)
        , pio_encode_set(pio_x, 31) | pio_encode_delay(29)
        , pio_encode_nop() | pio_encode_delay(29)
        , pio_encode_jmp_x_dec(6)
        };
    struct pio_program my_blink_prog = {
            .instructions = my_blink_prog_instr,
            .length = PROGRAM_LENGTH,
            .origin = -1,
           // .pio_version = 0,
    #if PICO_PIO_VERSION > 0
            .used_gpio_ranges = 0x0
    #endif
    };
    // Find a free pio and state machine and add the program
    //bool rc = pio_claim_free_sm_and_add_program_for_gpio_range(&my_blink_prog, &pio, &sm, &offset, PIO_BLINK_LED1_GPIO, 2, true);
    sm = pio_claim_unused_sm(pio, true);
    bool rc = pio_add_program(pio, &my_blink_prog);
    hard_assert(rc);
    //printf("Loaded program at %u on pio %u\n", offset, PIO_NUM(pio));

    // Start led1 flashing
    blink_pin_forever(pio, sm, offset, PIO_BLINK_LED1_GPIO, 4);

    pio_sm_unclaim(pio, sm + 1);
    //pio_remove_program_and_unclaim_sm(&my_blink_prog, pio, sm, offset);

    // the program exits but the pio keeps running!
    //printf("All leds should be flashing\n");
}

static term nif_pio_init(Context *ctx, int argc, term argv[])
{
    UNUSED(ctx);
    UNUSED(argc);

    VALIDATE_VALUE(argv[0], term_is_integer);
    int unused_pio_var = argv[0];
    if (UNLIKELY(unused_pio_var != 0)) {
        RAISE_ERROR(BADARG_ATOM);
    }
    pio_helper();
    return OK_ATOM;
}

static const struct Nif pio_init_nif = {
    .base.type = NIFFunctionType,
    .nif_ptr = nif_pio_init
};


const struct Nif *pio_nif_get_nif(const char *nifname)
{
    if (strcmp("pio:init/1", nifname) == 0 || strcmp("Elixir.PIO:init/1", nifname) == 0) {
        TRACE("Resolved platform nif %s ...\n", nifname);
        return &pio_init_nif;
    }
    return NULL;
}

REGISTER_NIF_COLLECTION(pio, NULL, NULL, pio_nif_get_nif)
