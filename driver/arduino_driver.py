from enum import IntEnum, auto
import logging
from socket import socket, AF_INET, SOCK_STREAM


class SPI_commands(IntEnum):
    WRSR = auto()
    WRITE = auto()
    READ = auto()
    WRDI = auto()
    RDSR = auto()
    WREN = auto()


class Arduino:
    CONNECTING = 0x00
    I2C_CONF = 0x01
    I2C_WRITE = 0x02
    I2C_READ = 0x03
    SPI_CONF = 0x04
    SPI_READ_WRITE = 0x05
    GPIO_WRITE = 0x06
    GPIO_READ = 0x07
    ADC_READ = 0x08
    DEBUG = 0x0F

    PACKET_SIZE_DELIMITER = 0x3A
    DEBUG_ADDRESS = 0x01

    def __init__(self, IP_address: str, TCP_port: int):
        self.client_socket = None
        self._tcp_bufferSize = 1024
        self.port = TCP_port
        self.ip = IP_address

    def connect(self):
        self.client_socket = socket(AF_INET, SOCK_STREAM)
        self.client_socket.settimeout(2)
        try:
            self.client_socket.connect((self.ip, self.port))
        except Exception as e:
            logging.error(f"Failed to connect to Arduino: {e}")
            self.disconnect()

        self.__validate_handle()

    def is_connected(self) -> bool:
        try:
            self.__validate_handle()
        except Exception:
            return False
        else:
            return True

    def disconnect(self):
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None

    def __validate_handle(self):
        try:
            self.send_arduino(self.CONNECTING)
            msg = self.client_socket.recv(self._tcp_bufferSize).decode("utf-8")
            logging.debug(f"Arduino response: {msg}")
            if msg != "acknowledged":
                self.disconnect()
        except Exception as e:
            logging.error(f"Failed to validate Arduino handle: {e}")
            self.disconnect()
            raise

    def send_arduino(self, command: int, address: int = None, data: bytearray = None):
        size_of_transaction = 3  # Default size sending all 0's
        if data:
            size_of_transaction += (
                len(data) - 1
            )  # Minus one to account for data not being 0
        else:
            data = bytearray(b"\0")
        if address is None:
            address = 0x00

        size_of_transaction_bytes = size_of_transaction.to_bytes(
            length=4, byteorder="big"
        )
        bytes_to_send = (
            size_of_transaction_bytes
            + bytearray([self.PACKET_SIZE_DELIMITER, command, address])
            + data
        )

        try:
            self.client_socket.sendall(bytes_to_send)
        except Exception as e:
            logging.error(f"Failed to send data to Arduino: {e}")

    def GPIO_Write(self, address: int, data: bytearray):
        command = self.GPIO_WRITE
        self.send_arduino(command, address, data)

    def GPIO_Read(self, address: int):
        command = self.GPIO_READ
        self.send_arduino(command, address)
        try:
            msg = self.client_socket.recv(self._tcp_bufferSize)
            print(f"GPIO_Read. Address: {address} Read: {msg}")
            return msg
        except Exception as e:
            print(e)

    def I2C_Config(self, clock: int):
        command = self.I2C_CONF
        address = 1
        data = clock.to_bytes(length=4, byteorder="big")
        self.send_arduino(command, address, data)

    def I2C_Read(self, address: int, read_num: int):
        command = self.I2C_READ
        data = read_num.to_bytes(length=4, byteorder="big")
        self.send_arduino(command, address, data)
        try:
            # msg = self.client_socket.recv(self._tcp_bufferSize).decode("utf-8", "ignore")
            msg = self.client_socket.recv(self._tcp_bufferSize)
            print(f"I2C_Read: {msg}")
        except Exception as e:
            print(e)

    def I2C_Write(self, address: int, data: bytearray):
        command = self.I2C_WRITE
        self.send_arduino(command, address, data)

    def SPI_Conf(self, SPI_mode: int, clock_speed: float):
        command = self.SPI_CONF
        address = SPI_mode
        data = int(clock_speed).to_bytes(length=4, byteorder="big")
        self.send_arduino(command, address, data)

    def SPI_Read_Write(self, address: int, data: bytearray):
        # first check if the SPI device is busy
        command = self.SPI_READ_WRITE
        self.send_arduino(command, address, bytearray([5, 0]))
        try:
            bits = self.client_socket.recv(self._tcp_bufferSize)
            num_read = 0
            while (bits[1] & 0b0001) and num_read != 5:
                self.send_arduino(command, address, bytearray([5, 0]))
                bits = self.client_socket.recv(self._tcp_bufferSize)
                num_read += 1
                if num_read == 5:
                    raise ValueError("A very specific bad thing happened.")
        except Exception as e:
            print(e)
        self.send_arduino(command, address, data)
        try:
            msg = self.client_socket.recv(self._tcp_bufferSize)
            print(f"SPI_read: {msg}")
            return msg
        except Exception as e:
            print(e)

    def ADC_Read(self, address: int):
        command = self.ADC_READ
        self.send_arduino(command, address)
        try:
            msg = self.client_socket.recv(self._tcp_bufferSize)
            # print(f"ADC_Read. Address: {address} Read: {msg}")
            return msg
        except Exception as e:
            print(e)

    def enable_debug(self, enable: bool):
        command = self.DEBUG
        if enable:
            data = (1).to_bytes(length=4, byteorder="big")
            self.send_arduino(command, 1, data)
        else:
            data = (0).to_bytes(length=4, byteorder="big")
            self.send_arduino(command, 1, data)


class ArduinoTester:
    def __init__(self):
        self.driver = None

    def setup(self):
        port = 4455
        ip = "192.168.200.69"
        self.driver = Arduino(IP_address = ip, TCP_port = port)
        self.driver.connect()

    def enable_debug(self, enable: bool):
        self.driver.enable_debug(enable)

    def I2C_test(self):
        # self.driver.I2C_Config(400000)
        self.driver.I2C_Write(56, bytearray([3, 207]))
        self.driver.I2C_Write(56, bytearray([2, 224]))
        self.driver.I2C_Write(56, bytearray([1, 207]))
        self.driver.I2C_Read(56, 1)

    def SPI_test(self):
        read_spi = self.driver.SPI_Read_Write(7, bytearray([SPI_commands.WREN]))
        # This reads the status register
        read_spi = self.driver.SPI_Read_Write(7, bytearray([SPI_commands.RDSR, 0]))

        read_spi = self.driver.SPI_Read_Write(
            7, bytearray([SPI_commands.WRITE, 0, 0, 7])
        )
        read_spi = self.driver.SPI_Read_Write(7, bytearray([SPI_commands.WREN]))
        read_spi = self.driver.SPI_Read_Write(
            7, bytearray([SPI_commands.WRITE, 0, 1, 8])
        )
        read_spi = self.driver.SPI_Read_Write(7, bytearray([SPI_commands.WREN]))
        read_spi = self.driver.SPI_Read_Write(
            7, bytearray([SPI_commands.WRITE, 0, 2, 9])
        )
        read_spi = self.driver.SPI_Read_Write(
            7, bytearray([SPI_commands.READ, 0, 0, 0, 0, 0])
        )

    def GPIO_test(self):
        byte_int = 0
        data_byte = bytearray(byte_int.to_bytes(length=1, byteorder="big"))
        self.driver.GPIO_Write(13, data_byte)
        # time.sleep(1)
        msg = self.driver.GPIO_Read(13)

    def ADC_test(self):
        for _ in range(10):
            average_adc = []
            logging.disable(logging.CRITICAL)
            for _ in range(250):
                average_adc.append(
                    int.from_bytes(self.driver.ADC_Read(1), "big") * 3.3 / 4095
                )
            logging.disable(logging.NOTSET)
            print(f"Read Voltage: {round((sum(average_adc) / len(average_adc)), 2)} V")

    def run_tests(self):
        self.setup()
        self.enable_debug(True)
        self.I2C_test()
        self.SPI_test()
        self.GPIO_test()
        self.enable_debug(False)
        self.ADC_test()
        self.enable_debug(True)
        self.driver.disconnect()


if __name__ == "__main__":
    logging.basicConfig(level=logging.DEBUG)
    tester = ArduinoTester()
    tester.run_tests()
