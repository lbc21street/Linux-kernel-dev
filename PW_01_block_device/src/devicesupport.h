//=================================================================================================
//
// \file    devicesupport.h
// \brief
// \author  lbc21street
//
//=================================================================================================
#pragma once

#ifdef __cplusplus
extern "C" {
#endif  // __cplusplus

//
//
//


//
//
//

[[nodiscard]] int PwbdpRegisterBlockDevice(void);

void PwbdpUnregisterBlockDevice(void);

int PwbdpCreateDisk(void);

void PwbdpDestroyDisk(void);



#ifdef __cplusplus
}
#endif  // __cplusplus

//=================================================================================================
