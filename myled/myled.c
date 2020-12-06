/*
     Copyright (C) 2020  Ryuichi Ueda and Satoru Negishi. All right reserved.
*/
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/delay.h>

MODULE_AUTHOR("Ryuuichi Ueda and Satoru Negishi");
MODULE_DESCRIPTION("driver for LED control");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.0.3");

static dev_t dev;
static struct cdev cdv;
static struct class *cls = NULL;
static volatile u32 *gpio_base = NULL;

static int led_gpio[3] = {23, 25, 26};
static int LED_gpio[2] = {27, 17};

static ssize_t led_write(struct file* filp, const char* buf, size_t count, loff_t* pos)
{
	char c; //読み込んだ字を入れる変数
	int n,m,j, k;
	if(copy_from_user(&c,buf,sizeof(char)))
			return -EFAULT;

//	printk(KERN_INFO "receive %c\n",c);
	if(c == '0') {
		for(n = 0; n < 3; n++)
			gpio_base[10] = 1 << led_gpio[n];
	}
	else if(c== '1') {
		for(n = 0; n < 3; n++) 
			gpio_base[7] = 1 << led_gpio[n];
	}
	else if(c == '2') {
		for(n = 0; n < 2; n++)
			gpio_base[10] = 1 << LED_gpio[n];
	}
	else if(c == '3') {
		for(n = 0; n < 2; n++)
			gpio_base[7] = 1 << LED_gpio[n];
	}
	else if(c=='4'){
		for(n = 0; n < 3; n++) {
			gpio_base[10] = 1 << led_gpio[n];
			if (n < 2)
				gpio_base[10] = 1 << LED_gpio[n];
		}

		for(n = 0; n < 2; n++) {
			j = 0, k=0;
			gpio_base[7] = 1 << led_gpio[j];
			gpio_base[7] = 1 << LED_gpio[k];
			ssleep(3);
			for(m = 0; m < 5; m ++) {
				gpio_base[10] = 1 << LED_gpio[k];
				msleep(300);
				gpio_base[7] = 1 << LED_gpio[k];
				msleep(300);
			}
			gpio_base[10] = 1 << LED_gpio[k];
			k++;
			gpio_base[7] = 1 << LED_gpio[k];
			ssleep(3);
			gpio_base[10] = 1 << led_gpio[j];
			j++;
			gpio_base[7] = 1 << led_gpio[j];
			ssleep(2);
			gpio_base[10] = 1 << led_gpio[j];
			j++;
			gpio_base[7] = 1 << led_gpio[j];
			ssleep(5);
			gpio_base[10] = 1 << led_gpio[j];
			gpio_base[10] = 1 << LED_gpio[k];
		}
	}

	return 1; //読み込んだ文字数を返す（この場合はダミーの１）
}

static ssize_t sushi_read(struct file* filp, char* buf, size_t count, loff_t* pos)
{
	int size = 0;
	char sushi[] = {'s','u','s','h','i',0x0A};
	if(copy_to_user(buf+size,(const char *)sushi, sizeof(sushi))){
		printk(KERN_ERR "sushi : copy_to_user failed\n");
		return -EFAULT;
	}
	size += sizeof(sushi);
	return size;
}

static struct file_operations led_fops = {
	.owner = THIS_MODULE,
	.write = led_write,
	.read = sushi_read
};

static int __init init_mod(void) //カーネルモジュールの初期化
{
	int retval;
	retval = alloc_chrdev_region(&dev, 0, 1, "myled");
	if(retval < 0) {
		printk(KERN_ERR "alloc_chrdev_region failed.\n");
		return retval;
	}
	printk(KERN_INFO"%s is loaded. major:%d\n",__FILE__, MAJOR(dev));

	cdev_init(&cdv, &led_fops);
	retval = cdev_add(&cdv, dev, 1);
	if(retval < 0) {
		printk(KERN_ERR "cdev_add failed. major:%d, minor:%d\n",MAJOR(dev), MINOR(dev));
		return retval;
	}
	
	cls = class_create(THIS_MODULE, "myled");
	if(IS_ERR(cls)){
		printk(KERN_ERR "class_create failed.");
		return PTR_ERR(cls);
	}

	device_create(cls, NULL, dev, NULL, "myled%d",MINOR(dev));

	gpio_base = ioremap_nocache(0xfe200000, 0xA0);

	int n;
	for(n = 0; n < 3; n++){
		const u32 led = led_gpio[n];
		const u32 index = led/10; //GPFSEL2
		const u32 shift = (led%10)*3; //15bit
		const u32 mask = ~(0x7 << shift); 
		gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);
	}

	for(n = 0; n < 2; n++){
		const u32 led = LED_gpio[n];
		const u32 index = led/10; //GPFSEL2
		const u32 shift = (led%10)*3; //15bit
		const u32 mask = ~(0x7 << shift); 
		gpio_base[index] = (gpio_base[index] & mask) | (0x1 << shift);
	}
	return 0;
}

static void __exit cleanup_mod(void) //後始末
{
	cdev_del(&cdv);
	device_destroy(cls, dev);
	class_destroy(cls);
	unregister_chrdev_region(dev, 1);
	printk(KERN_INFO"%s is unloaded. major:%d\n",__FILE__, MAJOR(dev));
}

module_init(init_mod); //マクロで関数を登録
module_exit(cleanup_mod); //同上
