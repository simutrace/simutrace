using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace csharp.memreplay
{
    [Serializable]
    internal class CommandLineException : Exception
    {
        public CommandLineException(string message) :
            base(message) { }
    }

    [Serializable]
    internal class ReplayException : Exception
    {
        public ReplayException(string message) :
            base(message) { }
    }

    static class Program
    {
        static private void _logger(string message)
        {
            Console.Write(message);
        }

        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        static void Main(string[] args)
        {
            try {
                // We first have to parse the command line arguments so we get
                // the connection information and any other parameters.
                Dictionary<string, string> values = new Dictionary<string, string>();
                Dictionary<string, bool> switches = new Dictionary<string, bool>();

                // Add options that expect a value here
                values.Add("-s", "local:/tmp/.simutrace"); // server
                values.Add("-m", "mem_cpu_store_dphys"); // memory stream name
                values.Add("-r", "512"); // Size of RAM to replay in MiB

                // Add switch options here
                switches.Add("-h", false);

                bool expectValue = false;
                string expectArg = String.Empty;
                for (int i = 0; i < args.Length; ++i) {

                    if (values.ContainsKey(args[i])) {
                        if (expectValue) {
                            break; // Error
                        }

                        expectValue = true;
                        expectArg = args[i];
                    } else if (switches.ContainsKey(args[i])) {
                        if (expectValue) {
                            break; // Error
                        }

                        switches[args[i]] = true;
                    } else if (expectValue) {
                        if (args[i][0] == '-') {
                            break;
                        }

                        expectValue = false;

                        values[expectArg] = args[i];
                    } else {
                        // This is potentially the unnamed argument at the end
                        if ((i == args.Length - 1) && (args[i][0] != '-')) {
                            values["unnamed"] = args[i];
                        } else {
                            throw new CommandLineException(
                                    String.Format("Unknown argument '{0}'", args[i]));
                        }
                    }
                }

                if (expectValue) {
                    throw new CommandLineException(String.Format(
                        "Expected value for argument '{0}'", expectArg));
                } else if (switches["-h"]) {
                    _printHelp();
                    return;
                } else if (!values.ContainsKey("unnamed")) {
                    throw new CommandLineException("Store name expected.");
                }

                uint ramSize;
                try {
                    ramSize = Convert.ToUInt32(values["-r"]);
                } catch (Exception e) {
                    if (e is FormatException || e is OverflowException) {
                        throw new CommandLineException("Invalid ram size.");
                    }

                    throw;
                }

                using (Replay replay = new Replay(_logger,
                                                  values["-s"],
                                                  values["unnamed"],
                                                  values["-m"],
                                                  ramSize)) {
                    Application.EnableVisualStyles();
                    Application.SetCompatibleTextRenderingDefault(false);
                    MainForm mainForm = new MainForm(replay);

                    ReplayForm replayForm = new ReplayForm(replay);
                    replayForm.Show();

                    ScreenForm screenForm = new ScreenForm(replay);
                    screenForm.Show();

                    Application.Run(mainForm);
                }

            } catch (CommandLineException e) {
                Console.WriteLine(e.Message);
                _printHelp();
            } catch (ReplayException e) {
                Console.WriteLine(e.Message);
            } catch (SimutraceException e) {
                Console.WriteLine(e.Message);
            }
        }

        static private void _printHelp()
        {
            Console.WriteLine("Usage: [-s server] [-r ramSizeInMiB] [-m memoryStreamName] store");
        }
    }
}
