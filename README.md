# Soft PWM Loadable Kernel Module


### Inserting and removing the module

Use the following command to insert the module into the kernel

```
sudo insmod soft_pwm.c
```

And to remove

```
rmmod soft_pwm
```

### Reading and writing values

On inserting the module, three devices will be registerd namely pwm_yaw, pwm_pitch and pwm_roll
in the ```sys/class/soft_pwm``` directory. Each device has the folowing attributes:
   
	```
	pulse
	```
	```
	period
	```

The pulse width of each device can be changed by writing a value between 1000 and 2000 to the ```pulse```
attribute. The current pulse width can also be read from it.

The period can be changed by writing to the ```period``` attribute. The current period can also be read
from it.

Example:
```
echo 1200 > pulse
```

```
cat > pulse
```
