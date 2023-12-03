#include "tusb.h"
#include "xinput_host.h"
#include <math.h>

#define PI 3.14159265

uint32_t buttons;
int16_t jsSpinner = 0;
int16_t lastAngle = 0;

extern void __not_in_flash_func(post_globals)(
  uint8_t dev_addr,
  int8_t instance,
  uint32_t buttons,
  uint8_t analog_1x,
  uint8_t analog_1y,
  uint8_t analog_2x,
  uint8_t analog_2y,
  uint8_t analog_l,
  uint8_t analog_r,
  uint32_t keys,
  uint8_t quad_x
);

const double Rad2Deg = 180.0 / PI;
const double Deg2Rad = PI / 180.0;
int16_t calcAngle(int16_t x, int16_t y);
uint8_t byteScaleAnalog(int16_t xbox_val);

//--------------------------------------------------------------------+
// USB X-input
//--------------------------------------------------------------------+
#if CFG_TUH_XINPUT
void tuh_xinput_report_received_cb(uint8_t dev_addr, uint8_t instance, uint8_t const *report, uint16_t len)
{
  xinputh_interface_t *xid_itf = (xinputh_interface_t *)report;
  xinput_gamepad_t *p = &xid_itf->pad;
  const char* type_str;
  switch (xid_itf->type)
  {
    case 1: type_str = "Xbox One";          break;
    case 2: type_str = "Xbox 360 Wireless"; break;
    case 3: type_str = "Xbox 360 Wired";    break;
    case 4: type_str = "Xbox OG";           break;
    default: type_str = "Unknown";
  }

  if (xid_itf->connected && xid_itf->new_pad_data)
  {
    printf("[%02x, %02x], Type: %s, Buttons %04x, LT: %02x RT: %02x, LX: %d, LY: %d, RX: %d, RY: %d\n",
           dev_addr, instance, type_str, p->wButtons, p->bLeftTrigger, p->bRightTrigger, p->sThumbLX, p->sThumbLY, p->sThumbRX, p->sThumbRY);

#ifdef CONFIG_NUON
    float max_thresh = 32768;
    float left1X = (128 * (p->sThumbLX / max_thresh)) + ((p->sThumbLX >= 0) ? 127 : 128);
    float left1Y = (128 * (-1 * p->sThumbLY / max_thresh)) + ((-1 * p->sThumbLY >= 0) ? 127 : 128);
    float left2X = (128 * (p->sThumbRX / max_thresh)) + ((p->sThumbRX >= 0) ? 127 : 128);
    float left2Y = (128 * (-1 * p->sThumbRY / max_thresh)) + ((-1 * p->sThumbRY >= 0) ? 127 : 128);

    if (p->sThumbLX == 0) left1X = 127;
    if (p->sThumbLY == 0) left1Y = 127;
    if (p->sThumbRX == 0) left2X = 127;
    if (p->sThumbRY == 0) left2Y = 127;

    // shift axis values for nuon
    uint8_t analog_1x = left1X+1;
    uint8_t analog_1y = left1Y+1;
    uint8_t analog_2x = left2X+1;
    uint8_t analog_2y = left2Y+1;
    if (analog_1x == 0) analog_1x = 255;
    if (analog_1y == 0) analog_1y = 255;
    if (analog_2x == 0) analog_2x = 255;
    if (analog_2y == 0) analog_2y = 255;

    // calc right thumb stick angle for simulated spinner
    if (analog_2x < 64 || analog_2x > 192 || analog_2y < 64 || analog_2y > 192) {
      int16_t angle = 0;
      angle = calcAngle(analog_2x-128, analog_2y-128)+179; // 0-359 (360deg)
      // printf("x: %d y: %d angle: %d \r\n", analog_2x-128, analog_2y-128, angle+180);

      // get directional difference delta
      int16_t delta = 0;
      if (angle >= lastAngle) delta = angle - lastAngle;
      else delta = (-1) * (lastAngle - angle);

      // check max/min delta value
      if (delta > 16) delta = 16;
      if (delta < -16) delta = -16;

      // inc global spinner value by delta
      jsSpinner -= delta;

      // check max/min spinner value
      if (jsSpinner > 255) jsSpinner -= 255;
      if (jsSpinner < 0) jsSpinner = 256 - (-1 * jsSpinner);

      lastAngle = angle;
    }
#else
    uint8_t analog_1x = byteScaleAnalog(p->sThumbLX);
    uint8_t analog_1y = byteScaleAnalog(p->sThumbLY);
    uint8_t analog_2x = byteScaleAnalog(p->sThumbRX);
    uint8_t analog_2y = byteScaleAnalog(p->sThumbRY);
#endif
    uint8_t analog_l = p->bLeftTrigger;
    uint8_t analog_r = p->bRightTrigger;

    bool is6btn = true;

    buttons = (((p->wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) ? 0x00 : 0x20000) |
               ((p->wButtons & XINPUT_GAMEPAD_LEFT_THUMB) ? 0x00 : 0x10000) |
               ((p->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER) ? 0x00 : 0x8000) |
               ((p->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER) ? 0x00 : 0x4000) |
               ((p->wButtons & XINPUT_GAMEPAD_X) ? 0x00 : 0x2000) |
               ((p->wButtons & XINPUT_GAMEPAD_Y) ? 0x00 : 0x1000) |
               ((is6btn) ? 0x00 : 0x0800) |
               ((false)  ? 0x00 : 0x0400) | // TODO: parse guide button report
               ((analog_r > 200) ? 0x00 : 0x0200) | // R2
               ((analog_l > 200) ? 0x00 : 0x0100) | // L2
               ((p->wButtons & XINPUT_GAMEPAD_DPAD_LEFT) ? 0x00 : 0x08) |
               ((p->wButtons & XINPUT_GAMEPAD_DPAD_DOWN) ? 0x00 : 0x04) |
               ((p->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) ? 0x00 : 0x02) |
               ((p->wButtons & XINPUT_GAMEPAD_DPAD_UP) ? 0x00 : 0x01) |
               ((p->wButtons & XINPUT_GAMEPAD_START) ? 0x00 : 0x80) |
               ((p->wButtons & XINPUT_GAMEPAD_BACK) ? 0x00 : 0x40) |
               ((p->wButtons & XINPUT_GAMEPAD_A) ? 0x00 : 0x20) |
               ((p->wButtons & XINPUT_GAMEPAD_B) ? 0x00 : 0x10));

    post_globals(dev_addr, instance, buttons, analog_1x, analog_1y, analog_2x, analog_2y, analog_l, analog_r, 0, jsSpinner);
  }
  tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_mount_cb(uint8_t dev_addr, uint8_t instance, const xinputh_interface_t *xinput_itf)
{
  printf("XINPUT MOUNTED %02x %d\n", dev_addr, instance);
  // If this is a Xbox 360 Wireless controller we need to wait for a connection packet
  // on the in pipe before setting LEDs etc. So just start getting data until a controller is connected.
  if (xinput_itf->type == XBOX360_WIRELESS && xinput_itf->connected == false)
  {
    tuh_xinput_receive_report(dev_addr, instance);
    return;
  }
  // tuh_xinput_init_chatpad(dev_addr, instance, true);
  tuh_xinput_set_led(dev_addr, instance, 0, true);
  // tuh_xinput_set_rumble(dev_addr, instance, 0, 0, true);
  tuh_xinput_receive_report(dev_addr, instance);
}

void tuh_xinput_umount_cb(uint8_t dev_addr, uint8_t instance)
{
  printf("XINPUT UNMOUNTED %02x %d\n", dev_addr, instance);
}

uint8_t byteScaleAnalog(int16_t xbox_val) {
  // Scale the xbox value from [-32768, 32767] to [1, 255]
  // Offset by 32768 to get in range [0, 65536], then divide by 256 to get in range [1, 255]
  uint8_t scale_val = (xbox_val + 32768) / 256;
  if (scale_val == 0) return 1;
  return scale_val;
}

int16_t calcAngle(int16_t x, int16_t y)
{
  return atan2(y, x) * Rad2Deg;
}

#endif
