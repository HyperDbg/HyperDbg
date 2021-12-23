using Microsoft.CodeAnalysis.CSharp.Scripting;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EvalScripts
{
    class Eval
    {
        public static async Task<Tuple<bool, int>> EvalStatementAsync(string Statement)
        {
            try
            {
                var SetParam = await CSharpScript.RunAsync("var num = " + Statement + ";");
                var RunCode = await SetParam.ContinueWithAsync("num");
                var X = RunCode.ReturnValue;

                //
                // Script run without error
                //
                return new Tuple<bool, int>(true, (int)X);
            }
            catch (Exception)
            {

                //
                // Script has error
                //
                return new Tuple<bool, int>(false, 0);
            }

        }

        public static async Task<Tuple<bool, int>> EvalScriptRunConditionalStatementAsync(string Script)
        {
            try
            {
                string FinalCode = @"
                
                int tmp = 0;
                void test_statement(int Token) {
                    tmp = Token;
                }
                    
                " + Script + @"
                
                int num = tmp;

                ";
                var SetParam = await CSharpScript.RunAsync(FinalCode);
                var RunCode = await SetParam.ContinueWithAsync("num");
                var X = RunCode.ReturnValue;

                //
                // Script run without error
                //
                return new Tuple<bool, int>(true, (int)X);
            }
            catch (Exception)
            {

                //
                // Script has error
                //
                return new Tuple<bool, int>(false, 0);
            }
        } 
        
        public static async Task<Tuple<bool, int>> EvalScriptRunLoopsAsync(string Script)
        {
            try
            {
                string FinalCode = @"
                
                int tmp = 0;
                int val = 0;
                void test_statement(int Token) {
                    tmp = Token;
                }

                int tmp_counter = 0;
                int x = 0;
                    
                " + Script + @"

                test_statement(tmp_counter);     
                int num = tmp;

                ";

                var SetParam = await CSharpScript.RunAsync(FinalCode);
                var RunCode = await SetParam.ContinueWithAsync("num");
                var X = RunCode.ReturnValue;

                //
                // Script run without error
                //
                return new Tuple<bool, int>(true, (int)X);
            }
            catch (Exception)
            {

                //
                // Script has error
                //
                return new Tuple<bool, int>(false, 0);
            }
        } 
        
        public static Tuple<bool, int> EvalScriptRun(string Script)
        {
            try
            {
                CSharpScript.EvaluateAsync(Script);

                //
                // Script run without error
                //
                return new Tuple<bool, int>(true, 0);
            }
            catch (Exception)
            {

                //
                // Script has error
                //
                return new Tuple<bool, int>(false, 0);
            }
        }
    }
}
