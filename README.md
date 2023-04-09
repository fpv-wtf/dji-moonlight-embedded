# dji-moonlight-embedded

Stream games via Moonlight and [fpv.wtf](https://github.com/fpv-wtf) to your DJI
FPV Goggles!

The DJI Moonlight project is made up of three parts:

- **[dji-moonlight-shim](https://github.com/fpv-wtf/dji-moonlight-shim)**: a
  goggle-side app that displays a video stream coming in over USB.
- **[dji-moonlight-gui](https://github.com/fpv-wtf/dji-moonlight-gui)**: a
  Windows app that streams games to the shim via Moonlight and friends.
- [dji-moonlight-embedded](https://github.com/fpv-wtf/dji-moonlight-embedded): a
  fork of Moonlight Embedded that can stream to the shim. The GUI app uses this
  internally. _You are here._

---

**Looking for easy setup? You'll probably want to use the [GUI app](https://github.com/fpv-wtf/dji-moonlight-gui) instead of this!**

This is a stripped down version of [moonlight-embedded](https://github.com/moonlight-stream/moonlight-embedded) that just yeets encoded
frames over the BULK or RNDIS to the shim.
