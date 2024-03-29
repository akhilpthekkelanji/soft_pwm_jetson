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

On inserting the module, three devices will be registerd namely pwm_yaw, pwm_pitch and pwm_roll in the
```sys/class/soft_pwm``` directory. Each device has the folowing attributes:


1.```pulse```

2.```period```

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
### pwm_tx2_src_mod

This is a modified version of the LKM that can be loaded on PC as well as TX2. It does not claim access
to GPIO pins when loaded on PC. Instead, a third attribute ```lp_stat``` outputs the number of times
the PWM generating loop has run. This can be used to check whether the loop is running or not.
