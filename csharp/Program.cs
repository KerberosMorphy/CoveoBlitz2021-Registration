using CommandLine;
using Microsoft.AspNetCore;
using Microsoft.AspNetCore.Hosting;
using Microsoft.Extensions.Configuration;
using Microsoft.Extensions.Hosting;
using System.Collections.Generic;

namespace Blitz
{
    public class Program
    {
        public static int port;

        class Options
        {
            [Option('p', "port", HelpText = "Set port number (default: 27178)", Default = 27178)]
            public int Port { get; set; }
        }

        public static void Main(string[] args)
        {
            Parser.Default.ParseArguments<Options>(args).WithParsed(o =>
            {
                port = o.Port;
            });

            CreateHostBuilder(args).Build().Run();
        }


        public static IHostBuilder CreateHostBuilder(string[] args) =>
            Host.CreateDefaultBuilder(args)
                .ConfigureWebHostDefaults(webBuilder =>
                {
                    webBuilder.UseStartup<Startup>();
                    webBuilder.UseUrls("http://localhost:" + port);
                    webBuilder.ConfigureAppConfiguration((context, config) =>
                    {
                        config.AddInMemoryCollection(new Dictionary<string, string>
                        {
                            {"Logging:LogLevel:System", "Error"},
                            {"Logging:LogLevel:Microsoft", "Error"}
                        });
                    });
                });
    }
}