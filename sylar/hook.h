#ifndef HOOK_H
#define HOOK_H

namespace sylar {

/**
 * @brief 当前线程是否hook 
 */
bool is_hook_enable();

/**
 * @brief 设置当前线程为hook 
 */
void set_hook_enable(bool flag);



} // namespace sylar

#endif