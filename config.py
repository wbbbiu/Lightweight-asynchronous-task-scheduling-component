import os

# 编译器系统头文件路径（保持你的原始路径不变）
SYSTEM_INCLUDES = [
    "C:/Program Files/Arm/gcc-arm-none-eabi/arm-none-eabi/include",
    "C:/Program Files/Arm/gcc-arm-none-eabi/lib/gcc/arm-none-eabi/15.2.1/include"
]

# 公共编译参数（所有 ARM Cortex-M 芯片通用）
COMMON_FLAGS = [
    "--target=arm-none-eabi",
    "-mthumb",
    "-std=gnu11"
]

# 核心精选：工业界最常用芯片配置 (严格校验，确保无误)
CHIP_CONFIGS = {
    # === F1 系列 (Cortex-M3, 无硬件浮点) ===
    "1": {
        "name": "STM32F103 中容量 (C8T6 / CBBT6 等) - [标准库]",
        "flags": ["-mcpu=cortex-m3", "-mfloat-abi=soft", "-DUSE_STDPERIPH_DRIVER", "-DSTM32F10X_MD"]
    },
    "2": {
        "name": "STM32F103 大容量 (ZET6 / VET6 等) - [标准库]",
        "flags": ["-mcpu=cortex-m3", "-mfloat-abi=soft", "-DUSE_STDPERIPH_DRIVER", "-DSTM32F10X_HD"]
    },
    "3": {
        "name": "STM32F103 全系列 - [HAL库 / CubeMX]",
        "flags": ["-mcpu=cortex-m3", "-mfloat-abi=soft", "-DUSE_HAL_DRIVER", "-DSTM32F103xE"] 
    },
    
    # === F4 系列 (Cortex-M4, 带单精度硬件浮点) ===
    "4": {
        "name": "STM32F407 / F405 系列 (ZGT6 / VGT6 等) - [标准库]",
        "flags": ["-mcpu=cortex-m4", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-DUSE_STDPERIPH_DRIVER", "-DSTM32F40_41xxx"]
    },
    "5": {
        "name": "STM32F407 / F405 系列 (ZGT6 / VGT6 等) - [HAL库 / CubeMX]",
        "flags": ["-mcpu=cortex-m4", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-DUSE_HAL_DRIVER", "-DSTM32F407xx"]
    },
    "6": {
        "name": "STM32F411 系列 (CEU6 等网红芯片) - [HAL库 / CubeMX]",
        "flags": ["-mcpu=cortex-m4", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-DUSE_HAL_DRIVER", "-DSTM32F411xE"]
    },

    # === 高阶/电机控制系列 (工作中常遇) ===
    "7": {
        "name": "STM32G431 (KBU6 等电机/电源神U) - [HAL库 / CubeMX]",
        "flags": ["-mcpu=cortex-m4", "-mfpu=fpv4-sp-d16", "-mfloat-abi=hard", "-DUSE_HAL_DRIVER", "-DSTM32G431xx"]
    },
    "8": {
        "name": "STM32H750 / H743 (强悍性能 Cortex-M7) - [HAL库 / CubeMX]",
        # H7 系列具有双精度浮点单元 (fpv5-d16)
        "flags": ["-mcpu=cortex-m7", "-mfpu=fpv5-d16", "-mfloat-abi=hard", "-DUSE_HAL_DRIVER", "-DSTM32H750xx"]
    }
}

def find_include_directories(root_path):
    """递归扫描所有包含 .h 文件的目录"""
    print("🔍 正在全速扫描工程中的头文件...")
    include_dirs = set()
    
    for dirpath, dirnames, filenames in os.walk(root_path):
        # 排除容易引发死循环或无用的隐藏目录/构建目录
        dirnames[:] = [d for d in dirnames if not d.startswith('.') and d not in ['build', 'Debug', 'Release']]
        
        if any(f.endswith('.h') for f in filenames):
            rel_path = os.path.relpath(dirpath, root_path)
            if rel_path == '.':
                include_dirs.add('.')
            else:
                include_dirs.add(rel_path.replace('\\', '/'))
                
    return sorted(list(include_dirs))

def main():
    print("="*55)
    print(" 🚀 终极版 STM32 clangd 自动配置脚本 (Trae / VS Code)")
    print("="*55)
    
    print("\n请选择你的目标芯片及库类型：")
    for key, config in CHIP_CONFIGS.items():
        print(f"  [{key}] {config['name']}")
    
    choice = input("\n👉 请输入数字并回车 (1-8): ").strip()
    if choice not in CHIP_CONFIGS:
        print("❌ 输入有误，未做任何修改，程序退出。")
        return
        
    selected_config = CHIP_CONFIGS[choice]
    print(f"\n✅ 已锁定配置: {selected_config['name']}")
    
    current_dir = os.path.abspath(os.path.dirname(__file__))
    include_dirs = find_include_directories(current_dir)
    print(f"✅ 扫描完毕！共揪出 {len(include_dirs)} 个包含头文件的文件夹。")
    
    output_file = os.path.join(current_dir, "compile_flags.txt")
    
    try:
        with open(output_file, 'w', encoding='utf-8') as f:
            # 1. 写入公共基础指令
            for flag in COMMON_FLAGS:
                f.write(f"{flag}\n")
                
            # 2. 写入特定芯片指令 (架构、FPU、宏定义)
            for flag in selected_config['flags']:
                f.write(f"{flag}\n")
            
            # 3. 写入编译器系统路径
            for sys_inc in SYSTEM_INCLUDES:
                f.write("-isystem\n")
                f.write(f"{sys_inc}\n")
                
            # 4. 写入工程自动扫描路径
            for inc in include_dirs:
                f.write(f"-I{inc}\n")
                
        print(f"\n🎉 完美！配置已生成至: {output_file}")
        print("⚡ 别忘了在 Trae 中按下 Ctrl+Shift+P，输入 'clangd: restart' 唤醒它！\n")
        
    except Exception as e:
        print(f"\n❌ 文件写入失败，发生意外状况: {e}")

if __name__ == "__main__":
    main()