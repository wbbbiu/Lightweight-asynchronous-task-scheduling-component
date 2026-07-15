#include "TimerWheel.h"

#include <string.h>

#include "MemoryPool.h"
#include "Memory_config.h"
#include "TimerWheel_config.h"
enum Del_MODE { DEL_MODE_FREE = 0, DEL_MODE_KEEP = 1 };

ERRCODE Timer_Node_Del(TimerWheel *timer, TimerNode **list, TimerNode *node,
                       int model);
ERRCODE Timer_Node_Run(TimerNode *node, TimerWheel *timer);

static TimerWheelOpt opt = {.Cancel_Task = Timer_Wheel_Cancel_Task,
                            .Submit_Task = Timer_Wheel_Submit_Task,
                            .Destory = Timer_Wheel_Destory,
                            .Show = Timer_Wheel_Show,
                            // .tick_run=Timer_Wheel_Run,
                            .Stop = Timer_Wheel_Stop,
                            .Loop = Timer_Wheel_Loop};

static inline uint32_t Make_Id(uint16_t memory_index, uint16_t version) {
  return ((uint32_t)memory_index << 16) | version;
}
static inline void *Parse_Id(MemoryPool *timer_pool, uint32_t id,
                             uint16_t *version) {
  uint16_t memory_index = id >> 16;
  *version = id & 0xFFFF;
  void *node = MemoryPool_GetByIndex(timer_pool, memory_index);
  return node;
}
void *TimerWheel_Alloc_Node(MemoryPool *timer_pool, uint16_t *memory_index) {
  void *node = MemoryPool_Alloc(timer_pool);
  if (!node) {
    ERROR(ERR_MEM);
    return NULL;
  }
  ERRCODE ret = MemoryPool_Index(timer_pool, node, memory_index);
  if (ret != ERR_OK) {
    MemoryPool_Free(timer_pool, node);
    node = NULL;
    ERROR(ret);
    return NULL;
  }
  return node;
}

// 根据这个精度和这个，槽数进行初始化
TimerWheel *Create_Timer_Wheel(uint16_t precision_ms, uint16_t slot_num,
                               uint32_t clock_mode, uint16_t max_timer_num) {
  if (precision_ms <= 0) {
    ERROR(ERR_ARGS, "精度必须大于0");
    return NULL;
  }
  if (!slot_num) {
    ERROR(ERR_ARGS, "槽数不能为0");
    return NULL;
  }

  int pos = 31 - __builtin_clz(slot_num);
  slot_num = (uint16_t)(1U << pos);
  // 分配这个内存给这个定时器
  TimerWheel *timer =
      malloc(sizeof(TimerWheel) + slot_num * sizeof(TimerNode *));
  if (!timer) {
    ERROR(ERR_MEM);
    return NULL;
  }
  // 创建内存池后面反复申请节点
  Create_MemoryPool(TimerNode, max_timer_num, &timer->timer_pool);

  if (!timer->timer_pool) {
    free(timer);
    timer = NULL;
    ERROR(ERR_MEM);
    return NULL;
  }
  // 槽对应的位移量
  timer->slot_bit = pos;
  // 时钟源确定
  timer->clock_mode = clock_mode;
  timer->active_count = 0;
  timer->current_slot = 0;
  timer->max_timer_num = max_timer_num;
  timer->slot_num = slot_num;
  // 时间片ms，也是基本精度
  timer->tick_ms = precision_ms;
  timer->completed_count = 0;
  timer->submit_count = 0;
  timer->cancel_count = 0;
  timer->is_shut = 0;
  timer->opt = opt;
  timer->last_slot = 0;
  timer->wheel_tick_remain_ms = 0;
  // 初始化这个定时器的槽,一开始全部为空
  for (int i = 0; i < slot_num; i++) {
    timer->slot[i] = NULL;
  }
  // 初始化这个定时器的节点,一开始全部为空
  MemoryPool_MemSet_Zero(timer->timer_pool);
  return timer;
}
/*
 * @brief 提交任务创建节点推算出所属槽，并插入初始化的节点
 * @param timer 定时器指针
 * @param task 任务指针
 * @param arg 任务参数
 * @param interval_ms 任务下一次触发的间隔时间单位 ms
 * @param repeat 任务重复次数，无限重复使用FOREVER，立即执行使用0
 * @return uint32_t 任务id,失败返回0，如果成功返回任务id
 * @note
 * 任务id是|index|版本号|index是这个节点地址在内存池中的下标，版本号是这个节点的版本号，
 */
ERRCODE Timer_Wheel_Submit_Task(TimerWheel *timer, void *(*task)(void *),
                                void *arg, uint32_t interval_ms, int repeat,
                                uint32_t *timer_id) {
  if (!timer) {
    return ERROR(ERR_PTR_NULL);
  }
  if (timer->is_shut) {
    return ERROR(ERR_OPT, "定时器已关闭");
  }
  if (!task) {
    return ERROR(ERR_ARGS, "任务指针不能为空");
  }
  if (timer_id) {
    *timer_id = TIMER_INVALID_ID;
  }
  if (repeat == 0) {
    timer->completed_count++;
    timer->submit_count++;
    if (task) {
      task(arg);
    }
    return ERR_OK;
  }
  // 计算出相对这个时间片的个数
  uint32_t ticks = (interval_ms + timer->tick_ms - 1) / timer->tick_ms;
  if (ticks == 0) {
    ticks = 1;
  }
  // 以当前槽位置为基准计算出对应槽位置
  uint16_t belong_slot = (timer->current_slot + ticks) & (timer->slot_num - 1);
  // 计算round,第多少轮触发
  uint32_t belong_round = (ticks - 1) >> timer->slot_bit;
  uint16_t memory_index = 0;
  // 节点分配内存
  TimerNode *node = TimerWheel_Alloc_Node(timer->timer_pool, &memory_index);
  if (node) {
    // 数据初始化
    node->arg = arg;
    node->callback = task;
    node->repeat = repeat;
    node->round = belong_round;
    node->memory_index = memory_index;
    node->interval_ms = interval_ms;
    // 插入链表 头插法
    ERRCODE ret = TimerNode_Push(&timer->slot[belong_slot], node);
    if (ret != ERR_OK) {
      MemoryPool_Free(timer->timer_pool, node);
      return ret;
    }
    node->version++;
    uint32_t id = Make_Id(memory_index, node->version);
    if (timer_id) {
      *timer_id = id;
    }
    timer->submit_count++;
    timer->active_count++;
    return ERR_OK;
  }
  return ERROR(ERR_MEM, "节点分配失败");
}
ERRCODE Timer_Wheel_Cancel_Task(TimerWheel *timer, uint32_t id) {
  if (!timer) {
    return ERROR(ERR_PTR_NULL);
  }
  if (timer->is_shut) {
    return ERROR(ERR_OPT, "定时器已关闭");
  }

  // 这个本身就是及时任务马上执行不会加入管理，所以没有删除操作
  if (id == TIMER_INVALID_ID) {
    return ERR_OK;
  }
  uint16_t version;
  TimerNode *node = (TimerNode *)Parse_Id(timer->timer_pool, id, &version);
  if (!node) {
    return ERROR(ERR_ARGS, "任务id不存在");
  }
  // 可能没有删除但是我们对比是不是同一个版本
  if (node->version != version || node->repeat == 0) {
    return ERROR(ERR_ARGS, "任务id不存在");
  }
  // 节点状态设置为删除
  node->repeat = 0;
  timer->cancel_count++;
  timer->active_count--;
  return ERR_OK;
}
ERRCODE Timer_Wheel_Loop(TimerWheel *timer) {
  if (!timer) {
    return ERROR(ERR_PTR_NULL);
  }
  if (timer->is_shut) {
    return ERROR(ERR_OPT, "定时器已关闭");
  }
  if (timer->clock_mode == TIMER_WHEEL_MODE_TIME) {
    TimerClock_Update(timer);
  }
  if (timer->current_slot == timer->last_slot) {
    return ERR_OK;
  }
  ERRCODE ret = ERR_OK;
  timer->last_slot = timer->current_slot;
  uint16_t slot_index = timer->current_slot;
  TimerNode *cur = timer->slot[slot_index]; // 取出槽
  while (cur) {                             // 遍历
    // 拷贝当前节点,并更改遍历指向因为这个过程我们会有删除节点
    // 所以这个节点更改需要再前面否则cur可能被删除无法更改
    TimerNode *opt_node = cur;
    cur = cur->next;
    // 这个节点该删除了
    if (opt_node->repeat == 0) {
      ret = Timer_Node_Del(timer, &timer->slot[slot_index], opt_node,
                           DEL_MODE_FREE);
      opt_node = NULL;
      continue;
    }
    if (opt_node->round == 0) {
      // 运行这个节点
      ret = Timer_Node_Run(opt_node, timer);
      if (opt_node->repeat) {
        ret = Timer_Node_Resubmit(opt_node, timer, slot_index);

      } // 使用完之后如果该删除就删除否则复用
      else {
        ret = Timer_Node_Del(timer, &timer->slot[slot_index], opt_node,
                             DEL_MODE_FREE);
        opt_node = NULL;
        timer->active_count--;
        timer->completed_count++;
      }
    } else {
      opt_node->round--;
    }
  }
  return ret;
}
void Timer_Wheel_Show(const TimerWheel *timer) {
  printf("软件定时器时间精度:%hu,槽数为:%u\n", timer->tick_ms, timer->slot_num);
  printf("使用情况-------\n");
  printf("当前排队任务数：%hu--------------------\n", timer->active_count);
  printf("当前累计完成数：%u--------------------\n", timer->completed_count);
  printf("当前累计提交数：%u--------------------\n", timer->submit_count);
  printf("当前累计取消数：%u--------------------\n", timer->cancel_count);
}
void Timer_Wheel_Destory(TimerWheel **timer) {
  if (!timer || !(*timer)) {
    return;
  }
  for (int i = 0; i < (*timer)->slot_num; i++) {
    while ((*timer)->slot[i]) {
      TimerNode *temp = (*timer)->slot[i];
      (*timer)->slot[i] = temp->next;
      if ((*timer)->slot[i])
        (*timer)->slot[i]->pre = NULL;
      MemoryPool_Free((*timer)->timer_pool, temp);
    }
  }
  MemoryPool_Destory(&(*timer)->timer_pool);
  free(*timer);
  *timer = NULL;
}

void Timer_Wheel_Stop(TimerWheel *timer) { timer->is_shut = 1; }
ERRCODE Timer_Node_Del(TimerWheel *timer, TimerNode **list, TimerNode *node,
                       int model) {
  if (!timer || !list || !node) {
    return ERROR(ERR_PTR_NULL);
  }
  if (!(*list)) {
    return ERROR(ERR_EMPTY);
  }
  ERRCODE ret = ERR_OK;
  TimerNode_Del(list, node);
  if (model == DEL_MODE_FREE) {
    ret = MemoryPool_Free(timer->timer_pool, node);
  }
  return ret;
}
ERRCODE Timer_Node_Run(TimerNode *node, TimerWheel *timer) {
  if (!node || !timer) {
    return ERROR(ERR_PTR_NULL);
  }
  // 如果没有回调函数,直接删除节点
  if (!node->callback) {
    node->repeat = 0;
    return ERR_OK;
  }
  node->callback(node->arg);
  // 如果不是永久循环,则重复次数减一
  if (node->repeat != FOREVER && node->repeat > 0)
    node->repeat--;
  return ERR_OK;
}
ERRCODE Timer_Node_Resubmit(TimerNode *node, TimerWheel *timer,
                            uint16_t cur_slot) {
  if (!node || !timer) {
    return ERROR(ERR_PTR_NULL);
  }
  uint32_t ticks = (node->interval_ms + timer->tick_ms - 1) / timer->tick_ms;
  if (ticks == 0) {
    ticks = 1;
  }
  uint16_t new_slot = (timer->current_slot + ticks) & (timer->slot_num - 1);
  node->round = (ticks - 1) >> timer->slot_bit;
  // 如果新的槽不是当前槽说明需要重新插入
  if (new_slot != cur_slot) {
    Timer_Node_Del(timer, &timer->slot[cur_slot], node, DEL_MODE_KEEP);
    return TimerNode_Push(&timer->slot[new_slot], node);
  }
  return ERR_OK;
}