#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <asm/mach/map.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <asm/io.h>
#include <linux/device.h>

#include <linux/platform_device.h>

/*------------------字符设备内容----------------------*/
#define DEV_NAME "rgb_led"
#define DEV_CNT (1)

/*定义 led 资源结构体，保存获取得到的节点信息以及转换后的虚拟寄存器地址*/
struct led_resource
{
	struct device_node *device_node; //rgb_led_red的设备树节点
	void __iomem *va_MODER;
	void __iomem *va_OTYPER;
	void __iomem *va_OSPEEDR;
	void __iomem *va_PUPDR;
	void __iomem *va_BSRR;
};

static void __iomem *va_clkaddr;

static dev_t led_devno;					 //定义字符设备的设备号
static struct cdev led_chr_dev;			 //定义字符设备结构体chr_dev
struct class *class_led;				 //保存创建的类
struct device *device;					 // 保存创建的设备
struct device_node *rgb_led_device_node; //rgb_led的设备树节点结构体

/*定义 R G B 三个灯的led_resource 结构体，保存获取得到的节点信息*/
struct led_resource led_red;
struct led_resource led_green;
struct led_resource led_blue;

/*字符设备操作函数集，open函数*/
static int led_chr_dev_open(struct inode *inode, struct file *filp)
{
	printk("\n open form driver \n");
	return 0;
}

/*字符设备操作函数集，write函数*/
static ssize_t led_chr_dev_write(struct file *filp, const char __user *buf, size_t cnt, loff_t *offt)
{

	unsigned int register_data = 0; //暂存读取得到的寄存器数据
	unsigned char write_data; //用于保存接收到的数据

	int error = copy_from_user(&write_data, buf, cnt);
	if (error < 0)
	{
		return -1;
	}
	// 开启a、b、g的时钟 
	writel(0x43, va_clkaddr);
	/*设置 GPIOA13 输出电平*/
	if (write_data & 0x04)
	{
		register_data |= (0x01 << (13+16));
		writel(register_data, led_red.va_BSRR); // GPIOA13引脚输出低电平，红灯亮
	}
	else
	{
		register_data |= (0x01 << (13));
		writel(register_data, led_red.va_BSRR); // GPIOA13引脚输出高电平，红灯灭
	}

	/*设置 GPIOG2 输出电平*/
	if (write_data & 0x02)
	{
		register_data |= (0x01 << (2+16));
		writel(register_data, led_green.va_BSRR); // GPIOG2引脚输出低电平，绿灯亮
	}
	else
	{
		register_data |= (0x01 << (2));
		writel(register_data, led_green.va_BSRR); // GPIOG2引脚输出高电平，绿灯灭
	}

	/*设置 GPIOB5 输出电平*/
	if (write_data & 0x01)
	{
		register_data |= (0x01 << (5+16));
		writel(register_data, led_blue.va_BSRR); //GPIOB5引脚输出低电平，蓝灯亮
	}
	else
	{
		register_data |= (0x01 << (5));
		writel(register_data, led_blue.va_BSRR); //GPIOB5引脚输出高电平，蓝灯灭
	}

	return 0;
}

/*字符设备操作函数集*/
static struct file_operations led_chr_dev_fops =
	{
		.owner = THIS_MODULE,
		.open = led_chr_dev_open,
		.write = led_chr_dev_write,
};

/*----------------平台驱动函数集-----------------*/
static int led_probe(struct platform_device *pdv)
{

	int ret = -1; //保存错误状态码
	unsigned int register_data = 0;

	printk(KERN_EMERG "\t  match successed  \n");

	/*获取rgb_led的设备树节点*/
	rgb_led_device_node = of_find_node_by_path("/rgb_led");
	if (rgb_led_device_node == NULL)
	{
		printk(KERN_ERR "\t  get rgb_led failed!  \n");
		return -1;
	}

	/*获取rgb_led节点的红灯子节点*/
	led_red.device_node = of_find_node_by_name(rgb_led_device_node,"rgb_led_red");
	if (led_red.device_node == NULL)
	{
		printk(KERN_ERR "\n get rgb_led_red_device_node failed ! \n");
		return -1;
	}


	/*获取 reg 属性并转化为虚拟地址*/
	led_red.va_MODER = of_iomap(led_red.device_node, 0);
	led_red.va_OTYPER = of_iomap(led_red.device_node, 1);
	led_red.va_OSPEEDR = of_iomap(led_red.device_node, 2);
	led_red.va_PUPDR = of_iomap(led_red.device_node, 3);
	led_red.va_BSRR = of_iomap(led_red.device_node, 4);
	va_clkaddr = of_iomap(led_red.device_node, 5);

	register_data = readl(va_clkaddr);
	// 开启a、b、g的时钟 
	register_data |= (0x43); // 开启a、b、g的时钟 
	writel(register_data, va_clkaddr);

	// 设置模式寄存器：输出模式
	register_data = readl(led_red.va_MODER);
	register_data &= ~((unsigned int)0X3 << (2 * 13));
	register_data |= ((unsigned int)0X1 << (2 * 13));
	writel(register_data,led_red.va_MODER);
	// 设置输出类型寄存器：推挽模式
	register_data = readl(led_red.va_OTYPER);
	register_data &= ~((unsigned int)0X1 << 13);
	writel(register_data, led_red.va_OTYPER);
	// 设置输出速度寄存器：高速
	register_data = readl(led_red.va_OSPEEDR);
	register_data &= ~((unsigned int)0X3 << (2 * 13));
	register_data |= ((unsigned int)0x2 << (2 * 13));
	writel(register_data, led_red.va_OSPEEDR);
	// 设置上下拉寄存器：上拉
	register_data = readl(led_red.va_PUPDR);
	register_data &= ~((unsigned int)0X3 << (2*13));
	register_data |= ((unsigned int)0x1 << (2*13));
	writel(register_data,led_red.va_PUPDR);
	// 设置置位寄存器：默认输出高电平
	register_data = readl(led_red.va_BSRR);
	register_data |= ((unsigned int)0x1 << (13));
	writel(register_data, led_red.va_BSRR);

	/*获取rgb_led节点的绿灯子节点*/
	led_green.device_node = of_find_node_by_name(rgb_led_device_node,"rgb_led_green");
	if (led_green.device_node == NULL)
	{
		printk(KERN_ERR "\n get rgb_led_green_device_node failed ! \n");
		return -1;
	}

	/*获取 reg 属性并转化为虚拟地址*/
	led_green.va_MODER = of_iomap(led_green.device_node, 0);
	led_green.va_OTYPER = of_iomap(led_green.device_node, 1);
	led_green.va_OSPEEDR = of_iomap(led_green.device_node, 2);
	led_green.va_PUPDR = of_iomap(led_green.device_node, 3);
	led_green.va_BSRR = of_iomap(led_green.device_node, 4);

	// 设置模式寄存器：输出模式
	register_data = readl(led_green.va_MODER);
	register_data &= ~((unsigned int)0X3 << (2 * 2));
	register_data |= ((unsigned int)0X1 << (2 * 2));
	writel(register_data,led_green.va_MODER);
	// 设置输出类型寄存器：推挽模式
	register_data = readl(led_green.va_OTYPER);
	register_data &= ~((unsigned int)0X1 << 2);
	writel(register_data, led_green.va_OTYPER);
	// 设置输出速度寄存器：高速
	register_data = readl(led_green.va_OSPEEDR);
	register_data &= ~((unsigned int)0X3 << (2 * 2));
	register_data |= ((unsigned int)0x2 << (2 * 2));
	writel(register_data, led_green.va_OSPEEDR);
	// 设置上下拉寄存器：上拉
	register_data = readl(led_green.va_PUPDR);
	register_data &= ~((unsigned int)0X3 << (2*2));
	register_data |= ((unsigned int)0x1 << (2*2));
	writel(register_data,led_green.va_PUPDR);
	// 设置置位寄存器：默认输出高电平
	register_data = readl(led_green.va_BSRR);
	register_data |= ((unsigned int)0x1 << (2));
	writel(register_data, led_green.va_BSRR);

	/*获取rgb_led节点的蓝灯子节点*/
	led_blue.device_node = of_find_node_by_name(rgb_led_device_node,"rgb_led_blue");
	if (led_blue.device_node == NULL)
	{
		printk(KERN_ERR "\n get rgb_led_blue_device_node failed ! \n");
		return -1;
	}

	/*获取 reg 属性并转化为虚拟地址*/
	led_blue.va_MODER = of_iomap(led_blue.device_node, 0);
	led_blue.va_OTYPER = of_iomap(led_blue.device_node, 1);
	led_blue.va_OSPEEDR = of_iomap(led_blue.device_node, 2);
	led_blue.va_PUPDR = of_iomap(led_blue.device_node, 3);
	led_blue.va_BSRR = of_iomap(led_blue.device_node, 4);

	// 设置模式寄存器：输出模式
	register_data = readl(led_blue.va_MODER);
	register_data &= ~((unsigned int)0X3 << (2 * 5));
	register_data |= ((unsigned int)0X1 << (2 * 5));
	writel(register_data,led_blue.va_MODER);
	// 设置输出类型寄存器：推挽模式
	register_data = readl(led_blue.va_OTYPER);
	register_data &= ~((unsigned int)0X1 << 5);
	writel(register_data, led_blue.va_OTYPER);
	// 设置输出速度寄存器：高速
	register_data = readl(led_blue.va_OSPEEDR);
	register_data &= ~((unsigned int)0X3 << (2 * 5));
	register_data |= ((unsigned int)0x2 << (2 * 5));
	writel(register_data, led_blue.va_OSPEEDR);
	// 设置上下拉寄存器：上拉
	register_data = readl(led_blue.va_PUPDR);
	register_data &= ~((unsigned int)0X3 << (2*5));
	register_data |= ((unsigned int)0x1 << (2*5));
	writel(register_data,led_blue.va_PUPDR);
	// 设置置位寄存器：默认输出高电平
	register_data = readl(led_blue.va_BSRR);
	register_data |= ((unsigned int)0x1 << (5));
	writel(register_data, led_blue.va_BSRR);

	/*---------------------注册 字符设备部分-----------------*/

	//第一步
	//采用动态分配的方式，获取设备编号，次设备号为0，
	//设备名称为rgb-leds，可通过命令cat  /proc/devices查看
	//DEV_CNT为1，当前只申请一个设备编号
	ret = alloc_chrdev_region(&led_devno, 0, DEV_CNT, DEV_NAME);
	if (ret < 0)
	{
		printk("fail to alloc led_devno\n");
		goto alloc_err;
	}
	//第二步
	//关联字符设备结构体cdev与文件操作结构体file_operations
	led_chr_dev.owner = THIS_MODULE;
	cdev_init(&led_chr_dev, &led_chr_dev_fops);
	//第三步
	//添加设备至cdev_map散列表中
	ret = cdev_add(&led_chr_dev, led_devno, DEV_CNT);
	if (ret < 0)
	{
		printk("fail to add cdev\n");
		goto add_err;
	}

	//第四步
	/*创建类 */
	class_led = class_create(THIS_MODULE, DEV_NAME);

	/*创建设备*/
	device = device_create(class_led, NULL, led_devno, NULL, DEV_NAME);

	return 0;

add_err:
	//添加设备失败时，需要注销设备号
	unregister_chrdev_region(led_devno, DEV_CNT);
	printk("\n error! \n");
alloc_err:

	return -1;
}

static const struct of_device_id rgb_led[] = {
	{.compatible = "fire,rgb_led"},
	{/* sentinel */}};

/*定义平台设备结构体*/
struct platform_driver led_platform_driver = {
	.probe = led_probe,
	.driver = {
		.name = "rgb-leds-platform",
		.owner = THIS_MODULE,
		.of_match_table = rgb_led,
	}};

/*
*驱动初始化函数
*/
static int __init led_platform_driver_init(void)
{
	int DriverState;
	DriverState = platform_driver_register(&led_platform_driver);
	printk(KERN_EMERG "\tDriverState is %d\n", DriverState);
	return 0;
}

/*
*驱动注销函数
*/
static void __exit led_platform_driver_exit(void)
{
	/*取消物理地址映射到虚拟地址*/
	iounmap(va_clkaddr);

	iounmap(led_green.va_MODER);
	iounmap(led_green.va_OTYPER);
	iounmap(led_green.va_OSPEEDR);
	iounmap(led_green.va_PUPDR);
	iounmap(led_green.va_BSRR);

	iounmap(led_red.va_MODER);
	iounmap(led_red.va_OTYPER);
	iounmap(led_red.va_OSPEEDR);
	iounmap(led_red.va_PUPDR);
	iounmap(led_red.va_BSRR);

	iounmap(led_blue.va_MODER);
	iounmap(led_blue.va_OTYPER);
	iounmap(led_blue.va_OSPEEDR);
	iounmap(led_blue.va_PUPDR);
	iounmap(led_blue.va_BSRR);

	/*删除设备*/
	device_destroy(class_led, led_devno);		  //清除设备
	class_destroy(class_led);					  //清除类
	cdev_del(&led_chr_dev);						  //清除设备号
	unregister_chrdev_region(led_devno, DEV_CNT); //取消注册字符设备

	/*注销字符设备*/
	platform_driver_unregister(&led_platform_driver);

	printk(KERN_EMERG "HELLO WORLD exit!\n");
}

module_init(led_platform_driver_init);
module_exit(led_platform_driver_exit);

MODULE_LICENSE("GPL");

/**/
