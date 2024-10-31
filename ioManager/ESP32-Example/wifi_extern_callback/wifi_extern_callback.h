#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

void wifi_example_regist_cb();
void wifi_example_on_connected(SemaphoreHandle_t handle, int* linkSuccessful);

#ifdef __cplusplus
}
#else

#endif