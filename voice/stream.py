import subprocess
import collections

def run_cpp_program():
    # Command to run the C++ program with specified arguments
    command = ['../whisper.cpp/stream', '-m', '../whisper.cpp/models/ggml-tiny.en.bin', '-t', '4', '--step', '1200', '--length', '1400', '-c', '0']

    # Create a deque (double-ended queue) to act as a FIFO queue
    output_fifo = collections.deque()

    # Run the C++ program and capture its output
    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)

    # Read the output line by line
    while True:
        output = process.stdout.readline()
        if output == '' and process.poll() is not None:
            break
        if output:
            # Process the output to remove commas, periods, and convert to lowercase
            processed_output = output.strip().replace(',', '').replace('.', '').replace('?', '').lower()
            print(processed_output)  # Print to console (optional)
            output_fifo.append(processed_output)

    # Wait for the process to finish and capture any remaining output
    stderr = process.communicate()[1]
    if stderr:
        print("Errors:\n", stderr)

    return output_fifo

if __name__ == '__main__':
    fifo = run_cpp_program()
    print("Captured output:")
    for line in fifo:
        print(line)
