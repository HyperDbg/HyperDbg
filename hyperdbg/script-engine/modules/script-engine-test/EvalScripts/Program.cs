using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Microsoft.CodeAnalysis.CSharp.Scripting;
using System.IO;

namespace EvalScripts
{
    class Program
    {
        enum ACTION_TYPE
        {
            CREATE_EXPRESSIONS,
            CREATE_CONDITIONAL_STATEMENTS,
            CREATE_CONDITIONAL_STATEMENTS_COMBINED_WITH_OTHER_STATEMENT,
            CREATE_FOR_LOOP,
            CREATE_FOR_LOOP_COMBINED_WITH_OTHER_STATEMENT,
            CREATE_WHILE_LOOP,
            CREATE_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT,
            CREATE_DO_WHILE_LOOP,
            CREATE_DO_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT,
        }

        private static void InitilizeIdentifers()
        {
            //
            // Registers
            //
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rax", Value = 0x1 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rdx", Value = 0x3 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rbx", Value = 0x4 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rsp", Value = 0x5 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rbp", Value = 0x6 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rsi", Value = 0x7 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@rdi", Value = 0x8 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r8", Value = 0x9 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r9", Value = 0xa });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r10", Value = 0xb });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r11", Value = 0xc });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r12", Value = 0xd });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "@r13", Value = 0xe });

            //
            // Pseudo-registers
            //
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "$proc", Value = 0x0 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "$thread", Value = 0x0 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "$teb", Value = 0x0 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "$ip", Value = 0x0 });
            StatementGenerator.Identifiers.Add(new StatementGenerator.IDENTIFIER_DEFINITION { IdentifierName = "$buffer", Value = 0x0 });

        }

        private static bool GenerateStatement
            (
            ACTION_TYPE Type,
            StreamWriter TestCaseWithErrorFile,
            StreamWriter TestCaseWithoutErrorFile,
            int Limit,
            UInt64 Counter
            )
        {

            string Result = string.Empty;
            string Sentence = string.Empty;
            string Script = string.Empty;
            bool EvalResult = false;

            StatementGenerator.ResetDepth();


            switch (Type)
            {
                case ACTION_TYPE.CREATE_EXPRESSIONS:

                    Sentence = StatementGenerator.EXPRESSION();

                    //
                    // Avoid big statements!
                    //
                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = "x = " + Sentence + "; test_statement(x);";
                    EvalResult = HighLevelScriptGen.EvaluateExpression(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_CONDITIONAL_STATEMENTS:

                    Sentence = StatementGenerator.IF_STATEMENT(false);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateConditionalStatement(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_CONDITIONAL_STATEMENTS_COMBINED_WITH_OTHER_STATEMENT:

                    Sentence = StatementGenerator.IF_STATEMENT(true);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateConditionalStatement(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_FOR_LOOP:

                    Sentence = StatementGenerator.FOR_STATEMENT(false);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_FOR_LOOP_COMBINED_WITH_OTHER_STATEMENT:

                    Sentence = StatementGenerator.FOR_STATEMENT(true);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_WHILE_LOOP:

                    Sentence = StatementGenerator.WHILE_STATEMENT(false);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT:

                    Sentence = StatementGenerator.WHILE_STATEMENT(true);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_DO_WHILE_LOOP:

                    Sentence = StatementGenerator.DO_WHILE_STATEMENT(false);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                case ACTION_TYPE.CREATE_DO_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT:

                    Sentence = StatementGenerator.DO_WHILE_STATEMENT(true);

                    if (!(Sentence.Length <= Limit))
                    {
                        return false;
                    }

                    Script = Sentence;
                    EvalResult = HighLevelScriptGen.EvaluateLoops(Sentence, ref Result);

                    break;

                default:

                    //
                    // Invalid value
                    //
                    return false;
            }


            Console.WriteLine(Counter + "\n\n" + Script + "\n\n" + Result + "\n");
            Console.WriteLine("------------------------------------------------------------");

            if (EvalResult)
            {
                TestCaseWithoutErrorFile.WriteLine(Counter.ToString());
                TestCaseWithoutErrorFile.WriteLine(Script);
                TestCaseWithoutErrorFile.WriteLine(Result);
                TestCaseWithoutErrorFile.WriteLine("$end$");

                return true;


            }
            else
            {
                TestCaseWithErrorFile.WriteLine(Counter.ToString());
                TestCaseWithErrorFile.WriteLine(Script);
                TestCaseWithErrorFile.WriteLine(Result);
                TestCaseWithErrorFile.WriteLine("$end$");

                return false;

            }
        }

        static void Generate(string CaseName, ACTION_TYPE Type, int MustCorrectCases, int CharacterLimit)
        {
            UInt64 Counter = 0;
            int CounterOfCorrectCases = 0;
            bool Correctness;

            //
            // Open files to save the results
            //
            StreamWriter TestCaseWithErrorFile = new StreamWriter(@"..\..\..\script-test-cases\" + CaseName + "-wrong.txt");
            StreamWriter TestCaseWithoutErrorFile = new StreamWriter(@"..\..\..\script-test-cases\" + CaseName + "-correct.txt");

            //
            // This the count of correct values
            //
            while (true)
            {
                Counter++;

                Correctness = GenerateStatement(Type, TestCaseWithErrorFile, TestCaseWithoutErrorFile, CharacterLimit, Counter);

                if (Correctness)
                {
                    CounterOfCorrectCases++;

                    if (CounterOfCorrectCases >= MustCorrectCases)
                    {
                        break;
                    }
                }
            }

            //
            // Close the files
            //
            TestCaseWithErrorFile.Close();
            TestCaseWithoutErrorFile.Close();
        }

        static void Main(string[] args)
        {
            //
            // First of all we should initialize identifiers and their values
            //
            InitilizeIdentifers();

            //
            // Generate test-cases
            //

             Generate("01-expressions", ACTION_TYPE.CREATE_EXPRESSIONS, 100000, 200);
             Generate("02-conditional-statements", ACTION_TYPE.CREATE_CONDITIONAL_STATEMENTS, 1000, 1000);
             Generate("03-conditional-statements-with-statements", ACTION_TYPE.CREATE_CONDITIONAL_STATEMENTS_COMBINED_WITH_OTHER_STATEMENT, 100, 5000);
             Generate("04-while-loop", ACTION_TYPE.CREATE_WHILE_LOOP, 1000, 200); 
             Generate("05-while-loop-with-statements", ACTION_TYPE.CREATE_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT, 100, 500); 
             Generate("06-do-while-loop", ACTION_TYPE.CREATE_DO_WHILE_LOOP, 1000, 500); 
             Generate("07-do-while-loop-with-statements", ACTION_TYPE.CREATE_DO_WHILE_LOOP_COMBINED_WITH_OTHER_STATEMENT, 300, 1000); 
             Generate("08-for-loop", ACTION_TYPE.CREATE_FOR_LOOP, 100, 200); // wrong
             Generate("09-for-loop-with-statements", ACTION_TYPE.CREATE_FOR_LOOP_COMBINED_WITH_OTHER_STATEMENT, 10, 200); // wrong

        }
    }
}
