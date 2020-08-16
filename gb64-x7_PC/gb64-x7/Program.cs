using System;
using System.IO;
using System.IO.Ports;
using System.Threading;

namespace gb64x7
{
    class Program
    {
        static SerialPort port;
        static int pbar_interval = 0; //(progress bar)
        static int pbar_ctr = 0;
        static string file;
        static int size = 0;

        static void Main(string[] args)
        {
            try
            {
                long time = DateTime.Now.Ticks;

                for (int i = 0; i < args.Length; i++)
                {
                    if (args[i].StartsWith("-port"))
                    {
                        connect(extractArg(args[i]));
                    }
                    
                    if (args[i].StartsWith("-file"))
                    {
                        file = extractArg(args[i]);
                    }
                    
                    if (args[i].StartsWith("-recv"))
                    {
                        size = Int32.Parse(extractArg(args[i]), System.Globalization.NumberStyles.HexNumber);
                        cmdRecv(file, size);
                    }
                    
                    if (args[i].StartsWith("-send"))
                    {
                        cmdSend(file);
                    }
                }

                time = (DateTime.Now.Ticks - time) / 10000;
                Console.WriteLine("time: {0:D}.{1:D3}", time / 1000, time % 1000);

            }
            catch (Exception x)
            {
                Console.WriteLine("");
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine("ERROR: " + x.Message);
                Console.ResetColor();
            }

            try
            {
                port.Close();
            }
            catch (Exception) { };
        }

        static void cmdRecv(string file, int len)
        {
            byte [] data = usbCmdRead(len);
            File.WriteAllBytes(file, data);
        }

        static void connect(string portName)
        {
            try
            {
                port = new SerialPort(portName);
                port.Open();
                Console.WriteLine("Port open on " + portName + "!");
                return;
            }
            catch (Exception) { }

            try
            {
                port.Close();
                port = null;
            }
            catch (Exception) { }
        }

        static void cmdSend(string file)
        {
            byte[] data = File.ReadAllBytes(file);
            usbCmdWrite(data);
        }

        static string extractArg(string cmd)
        {
            return cmd.Substring(cmd.IndexOf("=") + 1);
        }

        static byte[] usbCmdRead(int len)
        {
            Console.Write("Receiving.");
            pbar_interval = len > 0x2000000 ? 0x100000 : 0x80000;
            long time = DateTime.Now.Ticks;
            byte[] data = usbRead(len);
            time = DateTime.Now.Ticks - time;
            Console.WriteLine("ok. speed: " + getSpeedStr(data.Length, time));
            return data;
        }

        static byte[] usbCmdWrite(byte[] data)
        {
            int len = data.Length;

            Console.Write("Transmitting.");
            pbar_interval = len > 0x2000000 ? 0x100000 : 0x80000;
            long time = DateTime.Now.Ticks;
            usbWrite(data, 0, len);
            time = DateTime.Now.Ticks - time;

            Console.WriteLine("ok. speed: " + getSpeedStr(data.Length, time));

            return data;
        }

        static string getSpeedStr(long len, long time)
        {
            time /= 10000;
            if (time == 0) time = 1;
            long speed = ((len / 1024) * 1000) / time;

            return ("" + speed + " KB/s");
        }

        //********************************************************************************* 
        // USB communication
        //*********************************************************************************

        static void usbRead(byte[] data, int offset, int len)
        {
            while (len > 0)
            {
                int block_size = 32768;
                if (block_size > len) block_size = len;
                int readed = port.Read(data, offset, block_size);

                len -= readed;
                offset += readed;
                pbarUpdate(readed);
            }

            pbarReset();
        }

        static byte[] usbRead(int len)
        {
            byte[] data = new byte[len];
            usbRead(data, 0, data.Length);
            return data;
        }

        static void usbWrite(byte[] data, int offset, int len)
        {
            while (len > 0)
            {
                int block_size = 32768;
                if (block_size > len) block_size = len;
                port.Write(data, (int)offset, (int)block_size);
                len -= block_size;
                offset += block_size;
                pbarUpdate(block_size);
            }

            pbarReset();
        }

        static void usbWrite(byte[] data)
        {
            usbWrite(data, 0, data.Length);
        }

        static void pbarUpdate(int val)
        {
            if (pbar_interval == 0) return;
            pbar_ctr += val;
            if (pbar_ctr < pbar_interval) return;

            pbar_ctr -= pbar_interval;
            Console.Write(".");
        }

        static void pbarReset()
        {
            pbar_interval = 0;
            pbar_ctr = 0;
        }
    }
}