import numpy as np
from commands import *

class OneShotValid():
    def __init__(self, bitstream, infile, goldfile, outfile, args):
        self.bitstream = bitstream
        self.infile = infile
        self.goldfile = goldfile
        self.outfile = outfile
        self.args = args

    def commands(self):
        im = np.fromfile(
            self.infile,
            dtype=np.uint8
        ).astype(np.uint16)

        gold = np.fromfile(
            self.goldfile,
            dtype=np.uint8
        ).astype(np.uint16)

        return [
            WRITE_REG(GLOBAL_RESET_REG, 1),
            # Stall the CGRA
            WRITE_REG(STALL_REG, 0b1111),

            # Enable interrupts
            WRITE_REG(INTERRUPT_ENABLE_REG, 0b11),

            # WRITE_REG(CGRA_SOFT_RESET_EN_REG, 1),  # TODO: removeme
            # WRITE_REG(SOFT_RESET_DELAY_REG, 0),  # TODO: removeme

            # Configure the CGRA
            PRINT("Configuring CGRA..."),
            # *gc_config_bitstream(self.bitstream),
            *gb_config_bitstream(self.bitstream, width=self.args.width),
            PRINT("Done."),

            # # TODO: Do it again to test the interrupts, but remove later.
            # PRINT("Configuring CGRA..."),
            # # *gc_config_bitstream(self.bitstream),
            # *gb_config_bitstream(self.bitstream, width=self.args.width),
            # PRINT("Done."),

            # Set up global buffer for pointwise
            *configure_io(IO_INPUT_STREAM, BANK_ADDR(0), len(im), width=self.args.width),
            # TODO: would be better if this took in the input and
            # output tiles of the application and then configured the
            # io controllers appropriately.
            *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), io_ctrl=1, width=self.args.width),
            # *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), width=self.args.width),
            # *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(4), len(gold), width=self.args.width),

            # Put image into global buffer
            PRINT("Transferring input data..."),
            WRITE_DATA(BANK_ADDR(0), 0xc0ffee, im.nbytes, im),
            PRINT("Done."),

            # Start the application
            PRINT("Starting application..."),
            WRITE_REG(STALL_REG, 0),
            PEND(0b01, "start"),
            WRITE_REG(CGRA_START_REG, 1),
            PRINT("Waiting for completion..."),
            WAIT(0b01, "start"),
            PRINT("Done."),

            PRINT("Reading output data..."),
            READ_DATA(
                BANK_ADDR(16),
                gold.nbytes,
                gold,
                _file=self.outfile,
            ),
            PRINT("All tasks complete!"),
        ]

    def verify(self, result=None):
        print("Comparing outputs...")
        gold = np.fromfile(
            self.goldfile,
            dtype=np.uint8,
        )

        if result is None:
            result = np.fromfile(
                self.outfile,
                dtype=np.uint16,
            ).astype(np.uint8)

        if not np.array_equal(gold, result):
            if len(gold) != len(result):
                print(f"ERROR: Expected {len(gold)} outputs but got {len(result)}")
            for k, (x, y) in enumerate(zip(gold, result)):
                if x != y:
                    print(f"ERROR: [{k}] expected 0x{x:x} but got 0x{y:x}")
            return False

        print("Outputs match!")
        return True


class OneShotStall(OneShotValid):
    def commands(self):
        im = np.fromfile(
            self.infile,
            dtype=np.uint8
        ).astype(np.uint16)

        gold = np.fromfile(
            self.goldfile,
            dtype=np.uint8
        ).astype(np.uint16)

        return [
            WRITE_REG(GLOBAL_RESET_REG, 1),
            # Stall the CGRA
            WRITE_REG(STALL_REG, 0b1111),

            # Enable interrupts
            WRITE_REG(INTERRUPT_ENABLE_REG, 0b11),

            # WRITE_REG(CGRA_SOFT_RESET_EN_REG, 1),  # TODO: removeme
            # WRITE_REG(SOFT_RESET_DELAY_REG, 0),  # TODO: removeme

            # Configure the CGRA
            PRINT("Configuring CGRA..."),
            # *gc_config_bitstream(self.bitstream),
            *gb_config_bitstream(self.bitstream, width=self.args.width),
            PRINT("Done."),

            # # TODO: Do it again to test the interrupts, but remove later.
            # PRINT("Configuring CGRA..."),
            # # *gc_config_bitstream(self.bitstream),
            # *gb_config_bitstream(self.bitstream, width=self.args.width),
            # PRINT("Done."),

            # Set up global buffer for pointwise
            *configure_io(IO_INPUT_STREAM, BANK_ADDR(0), len(im), width=self.args.width),
            # TODO: would be better if this took in the input and
            # output tiles of the application and then configured the
            # io controllers appropriately.
            *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), io_ctrl=1, width=self.args.width),
            # *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), width=self.args.width),
            # *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(4), len(gold), width=self.args.width),

            # Put image into global buffer
            PRINT("Transferring input data..."),
            WRITE_DATA(BANK_ADDR(0), 0xc0ffee, im.nbytes, im),
            PRINT("Done."),

            # Start the application
            PRINT("Starting application..."),
            WRITE_REG(STALL_REG, 0),
            PEND(0b01, "start"),
            WRITE_REG(CGRA_START_REG, 1),

            WRITE_REG(STALL_REG, 0b1111),  # HACK
            STALL(50),  # HACK
            WRITE_REG(STALL_REG, 0),  # HACK

            STALL(100),  # HACK

            WRITE_REG(STALL_REG, 0b1111),  # HACK
            STALL(50),  # HACK
            WRITE_REG(STALL_REG, 0),  # HACK

            STALL(100),  # HACK

            WRITE_REG(STALL_REG, 0b1111),  # HACK
            STALL(50),  # HACK
            WRITE_REG(STALL_REG, 0),  # HACK

            PRINT("Waiting for completion..."),
            WAIT(0b01, "start"),
            PRINT("Done."),

            PRINT("Reading output data..."),
            READ_DATA(
                BANK_ADDR(16),
                gold.nbytes,
                gold,
                _file=self.outfile,
            ),
            PRINT("All tasks complete!"),
        ]


class Tiled():
    def __init__(self, bitstream, infiles, goldfiles, outfiles, args):
        self.bitstream = bitstream
        self.infiles = infiles
        self.goldfiles = goldfiles
        self.outfiles = outfiles
        self.args = args

    def commands(self):
        ims = [
            np.fromfile(
                infile,
                dtype=np.uint8
            ).astype(np.uint16)
            for infile in self.infiles
        ]

        golds = [
            np.fromfile(
                goldfile,
                dtype=np.uint8
            ).astype(np.uint16)
            for goldfile in self.goldfiles
        ]

        command_list = [
            WRITE_REG(GLOBAL_RESET_REG, 1),
            # Stall the CGRA
            WRITE_REG(STALL_REG, 0b1111),

            # Enable interrupts
            WRITE_REG(INTERRUPT_ENABLE_REG, 0b11),

            # WRITE_REG(CGRA_SOFT_RESET_EN_REG, 1),  # TODO: removeme
            # WRITE_REG(SOFT_RESET_DELAY_REG, 0),  # TODO: removeme

            # Configure the CGRA
            PRINT("Configuring CGRA..."),
            # *gc_config_bitstream(self.bitstream),
            *gb_config_bitstream(self.bitstream, width=self.args.width),
            PRINT("Done."),

            # # TODO: Do it again to test the interrupts, but remove later.
            # PRINT("Configuring CGRA..."),
            # # *gc_config_bitstream(self.bitstream),
            # *gb_config_bitstream(self.bitstream, width=self.args.width),
            # PRINT("Done."),
        ]

        in_addrs = [ BANK_ADDR(0) + 2048 * (k % 2) for k in range(len(ims)) ]
        out_addrs = [ BANK_ADDR(16) + 2048 * (k % 2) for k in range(len(golds)) ]

        for k in range(len(ims)):
            command_list += [
                PRINT(f"Loading input {k}..."),
                WRITE_DATA(in_addrs[k], 0xc0ffee, ims[k].nbytes, ims[k]),
                *configure_io(IO_INPUT_STREAM, in_addrs[k], len(ims[k]), width=self.args.width),
                *configure_io(IO_OUTPUT_STREAM, out_addrs[k], len(golds[k]), io_ctrl=1, width=self.args.width),
            ]

            if k == 0:
                command_list += [
                    WRITE_REG(CGRA_SOFT_RESET_EN_REG, 1),
                    WRITE_REG(STALL_REG, 0),
                    PEND(0b01, f"start"),
                    WRITE_REG(CGRA_START_REG, 1),
                ]
            else:
                command_list += [
                    WRITE_REG(CGRA_AUTO_RESTART_REG, 1),
                    PRINT(f"Waiting on {k-1}..."),
                    WAIT(0b01, f"start"),
                    PRINT(f"Reading output {k-1}..."),
                    READ_DATA(
                        out_addrs[k-1],
                        golds[k-1].nbytes,
                        golds[k-1],
                        _file=self.outfiles[k],
                    ),
                ]

        command_list += [
            PRINT(f"Waiting on {len(golds)-1}..."),
            WAIT(0b01, f"start"),
            PRINT(f"Reading output {len(golds)-1}..."),
            READ_DATA(
                out_addrs[len(golds)-1],
                golds[len(golds)-1].nbytes,
                golds[len(golds)-1],
                _file=self.outfiles[k],
            ),
            PRINT("All tasks complete!"),
        ]

        return command_list

    def verify(self, results=None):
        print("Comparing outputs...")
        golds = [
            np.fromfile(
                self.goldfiles[k],
                dtype=np.uint8,
            )
            for k in range(len(self.goldfiles))
        ]

        if results is None:
            results = [
                np.fromfile(
                    self.outfiles[k],
                    dtype=np.uint16,
                ).astype(np.uint8)
                for k in range(len(self.outfiles))
            ]

        for gold, result in zip(golds, results):
            if not np.array_equal(gold, result):
                if len(gold) != len(result):
                    print(f"ERROR: Expected {len(gold)} outputs but got {len(result)}")
                for k, (x, y) in enumerate(zip(gold, result)):
                    if x != y:
                        print(f"ERROR: [{k}] expected 0x{x:x} but got 0x{y:x}")
                return False

        print("Outputs match!")
        return True


class OuterProduct():
    def __init__(self, bitstream, weightfiles, infiles, goldfile, outfile, args):
        self.bitstream = bitstream
        self.weightfiles = weightfiles
        self.infiles = infiles
        self.goldfile = goldfile
        self.outfile = outfile
        self.args = args

    def commands(self):
        wts = [
            np.fromfile(
                weightfile,
                dtype=np.uint8
            ).astype(np.uint16)
            for weightfile in self.weightfiles
        ]

        ims = [
            np.fromfile(
                infile,
                dtype=np.uint8
            ).astype(np.uint16)
            for infile in self.infiles
        ]

        gold = np.fromfile(
            goldfile,
            dtype=np.uint8
        ).astype(np.uint16)

        command_list = [
            WRITE_REG(GLOBAL_RESET_REG, 1),
            # Stall the CGRA
            WRITE_REG(STALL_REG, 0b1111),

            # Enable interrupts
            WRITE_REG(INTERRUPT_ENABLE_REG, 0b11),

            # Configure the CGRA
            PRINT("Configuring CGRA..."),
            # *gc_config_bitstream(self.bitstream),
            *gb_config_bitstream(self.bitstream, width=self.args.width),
            PRINT("Done."),
        ]

        # Load weights to consecutive memory in global buffer
        wt_addr = BANK_ADDR(0)
        wt_len = 0
        for wt in wts:
            command_list += [
                PRINT(f"Loading weight {k}..."),
                WRITE_DATA(wt_addr, 0xc0ffee, wt.nbytes, wt),
            ]
            wt_addr += wt.nbytes
            wt_len += len(wt)

        # Load images to consecutive memory in global buffer
        im_addr = BANK_ADDR(4)
        im_len = 0
        for im in ims:
            command_list += [
                PRINT(f"Loading weight {k}..."),
                WRITE_DATA(im_addr, 0xc0ffee, im.nbytes, im),
            ]
            im_addr += im.nbytes
            im_len += len(im)

        command_list += [
            *configure_io(IO_INPUT_STREAM, BANK_ADDR(0), len(wts[0]), width=self.args.width),
            *configure_io(IO_INPUT_STREAM, BANK_ADDR(4), im_len, width=self.args.width),
            *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), width=self.args.width),

            # Run the application
            PRINT("Starting application..."),
            WRITE_REG(STALL_REG, 0),

            PEND(0b01, "start"),
            WRITE_REG(CGRA_START_REG, 1),
            # PRINT("Waiting for completion..."),
            # PRINT("Done."),
        ]

        # c code
        # wait until semaphore == 0
        # disable interrupts
        # signal semaphore
        # auto_restart = 1
        # enable interrupts

        # interrupt handler
        # if semaphore > 0
        #   if auto_restart == 1
        #     auto_restart = 0
        #     cgra_start = 1
        # decrement semaphore

        for k in range(1, len(wts)):
            command_list += [
                *configure_io(IO_INPUT_STREAM, BANK_ADDR(0) + k*len(wts[0]), len(wts[k]), width=self.args.width),
                *configure_io(IO_INPUT_STREAM, BANK_ADDR(4), im_len, width=self.args.width),
                WRITE_REG(CGRA_AUTO_RESTART_REG, 1),
                WAIT(0b01, "start"),
            ]

        command_list += [
            WAIT(0b01, "start"),
            PRINT("Reading output data..."),
            READ_DATA(
                BANK_ADDR(16),
                gold.nbytes,
                gold,
                _file=self.outfile,
            ),
            PRINT("All tasks complete!"),
        ]

        return command_list

    def verify(self, results=None):
        print("Comparing outputs...")
        gold = np.fromfile(
            self.goldfiles,
            dtype=np.uint8,
        )

        if results is None:
            result = np.fromfile(
                self.outfile,
                dtype=np.uint16,
            ).astype(np.uint8)

        if not np.array_equal(gold, result):
            if len(gold) != len(result):
                print(f"ERROR: Expected {len(gold)} outputs but got {len(result)}")
            for k, (x, y) in enumerate(zip(gold, result)):
                if x != y:
                    print(f"ERROR: [{k}] expected 0x{x:x} but got 0x{y:x}")
            return False

        print("Outputs match!")
        return True

class Conv3x3ReLU():
    def __init__(self, bitstream, weightfiles, infiles, goldfile, outfile, args):
        self.bitstream = bitstream
        self.weightfiles = weightfiles
        self.infiles = infiles
        self.goldfile = goldfile
        self.outfile = outfile
        self.args = args

    def commands(self):
        wts = [
            np.fromfile(
                weightfile,
                dtype=np.uint8
            ).astype(np.uint16)
            for weightfile in self.weightfiles
        ]

        ims = [
            np.fromfile(
                infile,
                dtype=np.uint8
            ).astype(np.uint16)
            for infile in self.infiles
        ]

        gold = np.fromfile(
            goldfile,
            dtype=np.uint8
        ).astype(np.uint16)

        command_list = [
            WRITE_REG(GLOBAL_RESET_REG, 1),
            # Stall the CGRA
            WRITE_REG(STALL_REG, 0b1111),

            # Enable interrupts
            WRITE_REG(INTERRUPT_ENABLE_REG, 0b11),

            # Configure the CGRA
            PRINT("Configuring CGRA..."),
            # *gc_config_bitstream(self.bitstream),
            *gb_config_bitstream(self.bitstream, width=self.args.width),
            PRINT("Done."),
        ]

        # Load weights to consecutive memory in global buffer
        wt_addr = BANK_ADDR(4)
        wt_len = 0
        for wt in wts:
            command_list += [
                PRINT(f"Loading weight {k}..."),
                WRITE_DATA(wt_addr, 0xc0ffee, wt.nbytes, wt),
            ]
            wt_addr += wt.nbytes
            wt_len += len(wt)

        # Load images to consecutive memory in global buffer
        im_addr = BANK_ADDR(0)
        im_len = 0
        for im in ims:
            command_list += [
                PRINT(f"Loading weight {k}..."),
                WRITE_DATA(im_addr, 0xc0ffee, im.nbytes, im),
            ]
            im_addr += im.nbytes
            im_len += len(im)

        in_chan = 64
        out_chan = 64
        in_x = 16
        in_y = 16
        out_x = 14
        out_y = 14

        for k in range(0, out_x * out_y):
            for j in range(0, 9):
                img_y = k + j // 3 - 1
                img_x = k + j % 3 - 1
                command_list += [
                    *configure_io(IO_INPUT_STREAM, BANK_ADDR(0) + (img_y * in_x + img_x) * in_chan, in_chan, width=self.args.width),
                ]
                for i in range(0, out_chan):
                    command_list += [
                        *configure_io(IO_INPUT_STREAM, BANK_ADDR(4) + j * out_chan * in_chan + i * in_chan, in_chan, width=self.args.width),
                    ]
                    if i == 0 and j == 0 and k == 0:
                        command_list += [
                            *configure_io(IO_OUTPUT_STREAM, BANK_ADDR(16), len(gold), width=self.args.width),
                
                            # Run the application
                            PRINT("Starting application..."),
                            WRITE_REG(STALL_REG, 0),
                
                            PEND(0b01, "start"),
                            WRITE_REG(CGRA_START_REG, 1),
                            # PRINT("Waiting for completion..."),
                            # PRINT("Done."),
                        ]
                    else:
                        command_list += [
                            WRITE_REG(CGRA_AUTO_RESTART_REG, 1), WAIT(0b01, "start"), ] 
                            WAIT(0b01, "start"),

        command_list += [
            WAIT(0b01, "start"),
            PRINT("Reading output data..."),
            READ_DATA(
                BANK_ADDR(16),
                gold.nbytes,
                gold,
                _file=self.outfile,
            ),
            PRINT("All tasks complete!"),
        ]

        return command_list

    def verify(self, results=None):
        print("Comparing outputs...")
        gold = np.fromfile(
            self.goldfiles,
            dtype=np.uint8,
        )

        if results is None:
            result = np.fromfile(
                self.outfile,
                dtype=np.uint16,
            ).astype(np.uint8)

        if not np.array_equal(gold, result):
            if len(gold) != len(result):
                print(f"ERROR: Expected {len(gold)} outputs but got {len(result)}")
            for k, (x, y) in enumerate(zip(gold, result)):
                if x != y:
                    print(f"ERROR: [{k}] expected 0x{x:x} but got 0x{y:x}")
            return False

        print("Outputs match!")
        return True
