import os
import subprocess

class config:
    yaw_gpio = "yaw"
    pitch_gpio = "pitch"
    roll_gpio = "roll"

def install_soft_pwm():
    ret = 0
    try:
        cmd = 'lsmod | grep soft_pwm'
        res = os.popen(cmd).read()
    except Exception as e:
        print("ERROR during lsmod command")
        print(e)
        return 1
    if 'soft_pwm' not in res:
        print('Installing soft_pwm kernel module')
        p = os.path.dirname(__file__)
        p = os.path.abspath(p)
        try:
            cmd = 'insmod ' + p + '/soft_pwm.ko'
            res = os.popen(cmd).read()
        except Exception as e:
            print("ERROR during insmod command")
            print(e)
            return 1
        if not ret:
            print("soft_pwm kernel module installed")
    else:
        print("soft_pwm already installed");

    return ret

def set_cpu_affinity():
    '''run pwm thread on CPU other than used for image processing'''
    try:
        pid = os.getpid()
        # do not run image processing on cpu 5
        os.system("sudo taskset -p --cpu-list 0-4 " + str(pid))
        # get pid of pwm task
        pid = subprocess.check_output(["pidof", "pwm_thread"])
        pid = str(pid)
        pid = pid[2:(len(pid)-3)]
        print("pid of pwm_thread = " + pid)
        # restrict it to cpu 5
        os.system("sudo taskset -p --cpu-list 5 " + pid)
    except Exception as e:
        print("ERRROR while setting cpu affinity")
        print(e)
        return 1
    return 0

def init_pwm_port(gpio):
    fname = '/sys/class/soft_pwm/pwm_' + gpio + '/pulse'
    #if not os.path.exists(fname):
     #   with open('/sys/class/soft_pwm/export', 'w') as f:
      #      f.write(gpio)
    with open(fname, 'w') as f:
        f.write("1500")

def init_soft_pwm():
    global config
    try:
        init_pwm_port(config.yaw_gpio)
        init_pwm_port(config.pitch_gpio)
        init_pwm_port(config.roll_gpio)
        return 0
    except Exception as e:
        print("ERROR configuring soft_pwm")
        print(e)
        return 1

def turn_motor(gpio, shift):
    if shift < 0:
        width = "1000"
    if shift > 0:
        width = "2000"
    if shift == 0:
        width = "1500"
    try:
        fname = '/sys/class/soft_pwm/pwm_' + gpio + '/pulse'
        with open(fname, 'w') as f:
            f.write(width)
    except Exception as e:
        print("ERROR setting duty cycle")
        print(e)

def turn_yaw(shift):
    global config
    turn_motor(config.yaw_gpio, shift)

def turn_pitch(shift):
    global config
    turn_motor(config.pitch_gpio, shift)

def turn_roll(shift):
    global config
    turn_motor(config.roll_gpio, shift)

def init():
    ret = install_soft_pwm()
    if not ret:
        ret = init_soft_pwm()
    if not ret:
        ret = set_cpu_affinity()
    return ret

if __name__ == '__main__':
    init()
    exit(0)
