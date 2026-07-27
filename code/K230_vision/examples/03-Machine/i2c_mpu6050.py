import machine
import time
import ustruct

def test_mpu6050():
    fpioa = machine.FPIOA()
    
    fpioa.set_function(11, machine.FPIOA.IIC2_SCL)
    fpioa.set_function(12, machine.FPIOA.IIC2_SDA)

    i2c = machine.I2C(2, freq=100000)
    print("I2C devices:", [hex(x) for x in i2c.scan()])
    
    # Initialize MPU6050
    i2c.writeto_mem(0x68, 0x6B, b'\x00')  # Wake up
    time.sleep_ms(200)
    
    # Verify registers
    whoami = i2c.readfrom_mem(0x68, 0x75, 1)
    print(f"WHO_AM_I: {hex(whoami[0])}")
    
    # Read loop
    while True:
        try:
            accel = i2c.readfrom_mem(0x68, 0x3B, 6)
            ax = ustruct.unpack('>h', accel[0:2])[0]
            ay = ustruct.unpack('>h', accel[2:4])[0]
            az = ustruct.unpack('>h', accel[4:6])[0]
            print(f"Accel: {ax} {ay} {az}")
            
            time.sleep(1)
        except Exception as e:
            print(f"Error: {e}")
            break

test_mpu6050()
