#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>

#define AHB4_PERIPH_BASE (0x50000000)

#define RCC_BASE (AHB4_PERIPH_BASE + 0x0000)	// 时钟控制寄存器
#define RCC_MP_GPIOENA (RCC_BASE + 0XA28)	// GPIO时钟使能寄存器

#define GPIOA_BASE (AHB4_PERIPH_BASE + 0x2000)	// GPIOA外设基地址
#define GPIOA_MODER (GPIOA_BASE + 0x0000)	// 模式寄存器
#define GPIOA_OTYPER (GPIOA_BASE + 0x0004)	// 输出类型寄存器
#define GPIOA_OSPEEDR (GPIOA_BASE + 0x0008) // 输出速度寄存器
#define GPIOA_PUPDR (GPIOA_BASE + 0x000C)	// 上下拉寄存器
#define GPIOA_BSRR (GPIOA_BASE + 0x0018)	// 置位寄存器

#define GPIOG_BASE (AHB4_PERIPH_BASE + 0x8000)
#define GPIOG_MODER (GPIOG_BASE + 0x0000)
#define GPIOG_OTYPER (GPIOG_BASE + 0x0004)
#define GPIOG_OSPEEDR (GPIOG_BASE + 0x0008)
#define GPIOG_PUPDR (GPIOG_BASE + 0x000C)
#define GPIOG_BSRR (GPIOG_BASE + 0x0018)

#define GPIOB_BASE (AHB4_PERIPH_BASE + 0x3000)
#define GPIOB_MODER (GPIOB_BASE + 0x0000)
#define GPIOB_OTYPER (GPIOB_BASE + 0x0004)
#define GPIOB_OSPEEDR (GPIOB_BASE + 0x0008)
#define GPIOB_PUPDR (GPIOB_BASE + 0x000C)
#define GPIOB_BSRR (GPIOB_BASE + 0x0018)

static struct resource rled_resource[] = {
	[0] = DEFINE_RES_MEM(GPIOA_MODER, 4),
	[1] = DEFINE_RES_MEM(GPIOA_OTYPER, 4),
	[2] = DEFINE_RES_MEM(GPIOA_OSPEEDR, 4),
	[3] = DEFINE_RES_MEM(GPIOA_PUPDR, 4),
	[4] = DEFINE_RES_MEM(GPIOA_BSRR, 4),
	[5] = DEFINE_RES_MEM(RCC_MP_GPIOENA, 4),
};

static struct resource gled_resource[] = {
	[0] = DEFINE_RES_MEM(GPIOG_MODER, 4),
	[1] = DEFINE_RES_MEM(GPIOG_OTYPER, 4),
	[2] = DEFINE_RES_MEM(GPIOG_OSPEEDR, 4),
	[3] = DEFINE_RES_MEM(GPIOG_PUPDR, 4),
	[4] = DEFINE_RES_MEM(GPIOG_BSRR, 4),
	[5] = DEFINE_RES_MEM(RCC_MP_GPIOENA, 4),
};

static struct resource bled_resource[] = {
	[0] = DEFINE_RES_MEM(GPIOB_MODER, 4),
	[1] = DEFINE_RES_MEM(GPIOB_OTYPER, 4),
	[2] = DEFINE_RES_MEM(GPIOB_OSPEEDR, 4),
	[3] = DEFINE_RES_MEM(GPIOB_PUPDR, 4),
	[4] = DEFINE_RES_MEM(GPIOB_BSRR, 4),
	[5] = DEFINE_RES_MEM(RCC_MP_GPIOENA, 4),
};
/* not used */ 
static void led_release(struct device *dev)
{

}

/* led hardware information */
unsigned int rled_hwinfo[2] = { 13, 0 };
unsigned int gled_hwinfo[2] = { 2, 6 };
unsigned int bled_hwinfo[2] = { 5, 1 };

/* red led device */ 
static struct platform_device rled_pdev = {
	.name = "led_pdev",
	.id = 0,
	.num_resources = ARRAY_SIZE(rled_resource),
	.resource = rled_resource,
	.dev = {
		.release = led_release,
		.platform_data = rled_hwinfo,
		},
};
/* green led device */ 
static struct platform_device gled_pdev = {
	.name = "led_pdev",
	.id = 1,
	.num_resources = ARRAY_SIZE(gled_resource),
	.resource = gled_resource,
	.dev = {
		.release = led_release,
		.platform_data = gled_hwinfo,
		},
};
/* blue led device */ 
static struct platform_device bled_pdev = {
	.name = "led_pdev",
	.id = 2,
	.num_resources = ARRAY_SIZE(bled_resource),
	.resource = bled_resource,
	.dev = {
		.release = led_release,
		.platform_data = bled_hwinfo,
		},
};

static __init int led_pdev_init(void)
{
	printk("pdev init\n");
	platform_device_register(&rled_pdev);
	platform_device_register(&gled_pdev);
	platform_device_register(&bled_pdev);
	return 0;
}

module_init(led_pdev_init);

static __exit void led_pdev_exit(void)
{
	printk("pdev exit\n");
	platform_device_unregister(&rled_pdev);
	platform_device_unregister(&gled_pdev);
	platform_device_unregister(&bled_pdev);
}

module_exit(led_pdev_exit);

MODULE_AUTHOR("embedfire");
MODULE_LICENSE("GPL");
