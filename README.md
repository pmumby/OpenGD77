# OpenGD77
Firmware for DMR transceivers using the NXP MK22 MCU, AT1846S RF chip and HR-C6000 DMR chipset. Including the Radioddiy GD-77, Baofeng DM-1801 and Baofeng RD-5R.


Note.
The AMBE codec is not open source because of potential copyright problems, and instead uses binary sections from original manufacturer’s firmware.

# Project status

The firmware is relatively stable and provides DMR and FM audio transmission and reception, as well as a DMR hotspot mode.
However it does currently support some core functionality that the official firmware supports, including sending and receiving of text messages, or Dual Capacity Direct operation, or Dual Watch operation etc

Not all functionality provided in the official firmware is likely ever to be part of this firmware, because some features like encryption are explicitly prohibited for Amateur Radio operators.

Some functionality like Dual Watch may also not ever be implemented, because it is equivalent to the Channel / Zone scan function.

The project is under active development by several developers.
This does not mean that new non-core functionality changes will be rejected, but bug fixes and core functionality PR’s will take priority

**All PR's should be submitted to the 'development' branch**


# User guide
There is an extensive User Guide https://github.com/rogerclarkmelbourne/OpenGD77/blob/master/docs/OpenGD77_User_Guide.md  however because the firmware is evolving very rapidly, sometimes the User Guide does show the latest features or functions.

# Credits
Originally conceived by Kai DG4KLU.
Further development by Roger VK3KYY, latterly assisted by Daniel F1RMB, Alex DL4LEX, Colin G4EML and others.

Current lead developer and source code gatekeeper is Roger VK3KYY


# License
This software is licensed under the GPL v2 and is intended for Amateur Radio and educational use only.

Use of this software for commercial purposes is strictly forbidden.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
