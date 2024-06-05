import subprocess
import collections
import lgpio

# GPIO setup using lgpio
ESTOP = [0, 1, 0, 0, 1]
rb1_gpio_assign = [23, 4, 17, 27, 22]
rb2_gpio_assign = [24, 10, 9, 11, 0]

# Open GPIO chip once and reuse the handle
h = lgpio.gpiochip_open(0)

def set_gpio_values(values, pins):
    try:
        for value, pin in zip(values, pins):
            lgpio.gpio_claim_output(h, pin)  # Claim the GPIO pin as output
            lgpio.gpio_write(h, pin, value)  # Set the GPIO pin to the specified value
    except Exception as e:
        print(f"Error setting GPIO values: {e}")

def run_cpp_program():
    # Command to run the C++ program with specified arguments
    command = ['../whisper.cpp/stream', '-m', '../whisper.cpp/models/ggml-tiny.en.bin', '-t', '4', '--step', '1010', '--length', '1010', '-c', '0']

    # Create a deque (double-ended queue) to act as a FIFO queue
    output_fifo = collections.deque()

    try:
        # Run the C++ program and capture its output
        process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

        # Read the output line by line
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                # Process the output to remove commas, periods, and convert to lowercase
                processed_output = output.strip().replace(',', '').replace('.', '').replace('!', '').lower()
                print(processed_output)  # Print to console
                output_fifo.append(processed_output)
                
                # Check for 'stop' keyword in the output
                if 'stop' in processed_output:
                    print("Stop command detected. Sending interrupt signal.")
                    set_gpio_values(ESTOP, rb1_gpio_assign)
                    set_gpio_values(ESTOP, rb2_gpio_assign)
                    lgpio.gpio_write(h, rb1_gpio_assign[0], 1)
                    lgpio.gpio_write(h, rb2_gpio_assign[0], 1)
                    break

        # Wait for the process to finish and capture any remaining output
        stderr = process.communicate()[1]
        if stderr:
            print("Errors:\n", stderr)

    except Exception as e:
        print(f"Error running C++ program: {e}")

    return output_fifo

if __name__ == '__main__':
    fifo = run_cpp_program()
    print("Captured output:")
    for line in fifo:
        print(line)

    # Clean up GPIO settings
    lgpio.gpiochip_close(h)
