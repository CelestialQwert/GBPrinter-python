from gbprinter import emulator
import logging

logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

emu = emulator.Emulator(palette = 'gray')

while True:
    emu.get_gb_data()