/*******************************************************************************
* Copyright (c) 2016, ROBOTIS CO., LTD.
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice, this
*   list of conditions and the following disclaimer.
*
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* * Neither the name of ROBOTIS nor the names of its
*   contributors may be used to endorse or promote products derived from
*   this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

/* 
 *  Authors: Taehoon Lim (Darby)
 *           Hancheol Cho (Baram) 
 *           Ashe Kim
 *           KyungWan Ki  (Kei)
 */

#include "RobotisRemoteController.h"

#ifdef SoftwareSerial_h
RobotisRemoteController::RobotisRemoteController(uint8_t rx_pin, uint8_t tx_pin)
  : p_hw_port_(nullptr), stream_port_(nullptr), enable_hw_port_(false)
{
  p_sw_port_ = new SoftwareSerial(rx_pin, tx_pin);
  stream_port_ = (Stream*) p_sw_port_;

  memset(&rc100_rx_, 0, sizeof(rc100_rx_));
}
#else
#pragma message("\r\nWarning : You CAN NOT use SoftwareSerial version of the RobotisRemoteController function(only HardwareSerial version enabled), because this board DOES NOT SUPPORT SoftwareSerial.h")
#pragma message("\r\nWarning : Please use RobotisRemoteController(HardwareSerial&) constructor only. (eg. RobotisRemoteController rc(Serial1);")
#endif

RobotisRemoteController::RobotisRemoteController(HardwareSerial& port)
  : p_hw_port_(&port), stream_port_(nullptr), enable_hw_port_(true)
{
#ifdef SoftwareSerial_h
  p_sw_port_ = nullptr;
#endif
  stream_port_ = (Stream*) p_hw_port_;
  memset(&rc100_rx_, 0, sizeof(rc100_rx_));
  begin();
}

RobotisRemoteController::~RobotisRemoteController()
{
}

void RobotisRemoteController::begin(uint32_t baudrate)
{
  rc100_rx_.state = 0;
  rc100_rx_.index = 0;
  rc100_rx_.received = false;
  rc100_rx_.released_event = false;

  if(enable_hw_port_){
    p_hw_port_ != nullptr ? p_hw_port_->begin(baudrate) : (void)(p_hw_port_);
  }else{
#ifdef SoftwareSerial_h
    p_sw_port_ != nullptr ? p_sw_port_->begin(baudrate) : (void)(p_sw_port_);
#endif  
  }
}

bool RobotisRemoteController::availableData(void)
{
  bool ret = false;

  if (stream_port_ == nullptr)
    return false;

  while(stream_port_->available() > 0)
  {
    ret = rc100Update(stream_port_->read());
  }

  return ret;
}

uint16_t RobotisRemoteController::readData(void)
{
  return rc100_rx_.data;
}

bool RobotisRemoteController::availableEvent(void)
{
  rc100_rx_.released_event = false;

  if (stream_port_ == nullptr)
    return false;

  while(stream_port_->available() > 0)
  {
    rc100Update(stream_port_->read());
  }
  
  return rc100_rx_.released_event;
}

uint16_t RobotisRemoteController::readEvent(void)
{
  return rc100_rx_.event_msg;
}

void RobotisRemoteController::flushRx(void)
{
  if (stream_port_ == nullptr)
    return;

  while(stream_port_->available())
  {
    stream_port_->read();
  }  
}

int RobotisRemoteController::available()
{
  if (stream_port_ == nullptr)
    return 0;

  return stream_port_->available();
}

int RobotisRemoteController::peek()
{
  if (stream_port_ == nullptr)
    return -1;

  return stream_port_->peek();
}

void RobotisRemoteController::flush()
{
  if (stream_port_ == nullptr)
    return;

  stream_port_->flush();
}

int RobotisRemoteController::read()
{
  if (stream_port_ == nullptr)
    return -1;

  return stream_port_->read();
}

size_t RobotisRemoteController::write(uint8_t data)
{
  if (stream_port_ == nullptr)
    return 0;

  return stream_port_->write(data);
}




bool RobotisRemoteController::rc100Update(uint8_t data)
{
  bool ret = false;
  static uint8_t save_data;
  static uint8_t inv_data;
  static uint32_t pre_time;

  inv_data = ~data;

  if (millis()-pre_time > 100)
  {
    rc100_rx_.state = 0;
  }

  switch(rc100_rx_.state)
  {
    case 0:
      if (data == 0xFF)
      {
        pre_time = millis();
        rc100_rx_.state = 1;
      }
      break;

    case 1:
      if (data == 0x55)
      {
        rc100_rx_.state    = 2;
        rc100_rx_.received = false;
        rc100_rx_.data     = 0;
      }
      else
      {
        rc100_rx_.state = 0;
      }
      break;

    case 2:
      rc100_rx_.data  = data;
      save_data      = data;
      rc100_rx_.state = 3;
      break;

    case 3:
      if (save_data == inv_data)
      {
        rc100_rx_.state = 4;
      }
      else
      {
        rc100_rx_.state = 0;
      }
      break;

    case 4:
      rc100_rx_.data |= data<<8;
      save_data      = data;
      rc100_rx_.state = 5;
      break;

    case 5:
      if (save_data == inv_data)
      {
        if (rc100_rx_.data > 0)
        {
          rc100_rx_.event_msg = rc100_rx_.data;
        }
        else
        {
          rc100_rx_.released_event = true;
        }
        rc100_rx_.received = true;
        ret = true;
      }
      rc100_rx_.state = 0;
      break;

    default:
      rc100_rx_.state = 0;
      break;
  }

  return ret;
}