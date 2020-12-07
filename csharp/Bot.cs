using Microsoft.AspNetCore.Mvc;
using Newtonsoft.Json;
using System;
using System.Collections.Generic;
using System.Threading;

namespace Blitz
{
    [ApiController]
    [Route("[controller]")]
    public class MyBot : ControllerBase
    {

        private static long[] answer;
        private static long[] track_cumsum;
        private static Problem problem;

        [HttpPost("/microchallenge")]
        public ActionResult Solve([FromBody] Problem p)
        {   
            problem = p;         
            answer = new long[problem.items.Length];
            track_cumsum = new long[problem.track.Length+1];
            
            long sums = 0;
            track_cumsum[0] = 0;
            for (long i = 1; i < problem.track.Length+1; i++)
            {
                sums += problem.track[i-1];
                track_cumsum[i] = sums;
            }

            if (answer.Length > 1000)
            {
                int width = answer.Length / 2;
                Thread[] threads = new Thread[2];
                
                threads[0] = new Thread(unused => job(0, width));
                threads[0].Start();
                
                threads[1] = new Thread(unused => job(width, answer.Length-width));
                threads[1].Start();

                threads[0].Join();
                threads[1].Join();
            }
            else
            {
                job(0, answer.Length);
            }

            return Ok(answer);
        }

        private static void job(int start, int count)
        {
            for (int i = start; i < start+count; i++)
                answer[i] = Math.Abs(track_cumsum[problem.items[i][1]]-track_cumsum[problem.items[i][0]]);
        }
    }

    public class Problem
    {
        public long[][] items { get; set; }
        public long[] track { get; set; }
    }    
}