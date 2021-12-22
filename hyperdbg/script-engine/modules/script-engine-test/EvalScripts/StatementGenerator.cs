using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace EvalScripts
{
    class StatementGenerator
    {
        //
        // Structures
        //
        public struct IDENTIFIER_DEFINITION
        {
            public string IdentifierName;
            public int Value;
        }

        //
        // Global variables and constants
        //
        static Random Rand = new Random();
        static int Depth = 0;
        const int MAX_DEPTH = 10;
        public const int SizeOfStructOfIdentifiers = 50;
        public static List<IDENTIFIER_DEFINITION> Identifiers = new List<IDENTIFIER_DEFINITION>();

        public static void ResetDepth()
        {
            Depth = 0;
        }

        public static string GET_CHECK_STATEMENT()
        {
            var token = Rand.Next(10000);
            string Result = string.Empty;

            Result = " test_statement(" + "0x" + token.ToString("X") + "); ";
            return Result;
        }

        public static string S()
        {

            var RandomNum = Rand.Next(2);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = STATEMENT() + " " + S();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }

        }

        public static string STATEMENT()
        {
            var RandomNum = Rand.Next(11);
            string Result = string.Empty;

            Depth += 1;

            switch (RandomNum)
            {
                case 0:

                    Result = IF_STATEMENT(false);
                    Depth -= 1;

                    return Result;

                case 1:

                    Result = WHILE_STATEMENT(false);
                    Depth -= 1;

                    return Result;

                case 2:

                    Result = DO_WHILE_STATEMENT(false);
                    Depth -= 1;

                    return Result;

                case 3:

                    Result = FOR_STATEMENT(false);
                    Depth -= 1;

                    return Result;

                case 4:

                    Result = ASSIGN_STATEMENT() + ";";
                    Depth -= 1;

                    return Result;

                case 5:

                    Result = CALL_FUNC_STATEMENT() + ";";
                    Depth -= 1;

                    return Result;

                case 6:

                    Result = " break;";
                    Depth -= 1;

                    return Result;

                case 7:

                    Result = " /* This is a comment */";
                    Depth -= 1;

                    return Result;

                case 8:

                    Result = " // /* This is a comment ";
                    Depth -= 1;

                    return Result;

                case 9:

                    Result = " /* This is a not closed comment";
                    Depth -= 1;

                    return Result;

                case 10:

                    Result = " This is a not opened comment */";
                    Depth -= 1;

                    return Result;

                default:

                    return string.Empty;

            }
        }

        public static string CALL_FUNC_STATEMENT()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string ASSIGN_STATEMENT()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = L_VALUE() + " = " + EXPRESSION() + " " + NULL();

            Depth -= 1;

            return Result;
        }

        public static string IF_STATEMENT(bool AddStatement)
        {
            string Result = string.Empty;

            Depth += 1;

            if (AddStatement)
            {
                Result = " if (" + BOOLEAN_EXPRESSION() + ")  {" + GET_CHECK_STATEMENT() + S() + "}" + ELSIF_STATEMENT(AddStatement) + ELSE_STATEMENT(AddStatement) + END_OF_IF();
            }
            else
            {
                Result = " if (" + BOOLEAN_EXPRESSION() + ")  {" + GET_CHECK_STATEMENT() + "}" + ELSIF_STATEMENT(AddStatement) + ELSE_STATEMENT(AddStatement) + END_OF_IF();
            }

            Depth -= 1;

            return Result;
        }

        public static string ELSIF_STATEMENT(bool AddStatement)
        {
            var RandomNum = Rand.Next(2);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {

                if (AddStatement)
                {
                    Result = " elsif (" + BOOLEAN_EXPRESSION() + ") {" + GET_CHECK_STATEMENT() + S() + "}" + ELSIF_STATEMENT(AddStatement);
                }
                else
                {
                    Result = " elsif (" + BOOLEAN_EXPRESSION() + ") {" + GET_CHECK_STATEMENT() + "}" + ELSIF_STATEMENT(AddStatement);
                }
                Depth -= 1;

                return Result;

            }
            else
            {

                Result = ELSIF_STATEMENTP();
                Depth -= 1;

                return Result;
            }
        }

        public static string ELSIF_STATEMENTP()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string ELSE_STATEMENT(bool AddStatement)
        {
            var RandomNum = Rand.Next(2);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                if (AddStatement)
                {
                    Result = " else {" + GET_CHECK_STATEMENT() + S() + "}";
                }
                else
                {
                    Result = " else {" + GET_CHECK_STATEMENT() + "}";
                }

                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }

        }

        public static string END_OF_IF()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string WHILE_STATEMENT(bool AddStatement)
        {
            string Result = string.Empty;

            Depth += 1;

            if (AddStatement)
            {
                Result = " while  (" + BOOLEAN_EXPRESSION() + ")  { tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } " + S() + " }";
            }
            else
            {
                Result = " while  (" + BOOLEAN_EXPRESSION() + ")  { tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } }";
            }

            Depth -= 1;

            return Result;
        }

        public static string DO_WHILE_STATEMENT(bool AddStatement)
        {
            string Result = string.Empty;

            Depth += 1;

            if (AddStatement)
            {
                Result = " do  {  tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } " + S() + "} while ( " + BOOLEAN_EXPRESSION() + ") ;";
            }
            else
            {
                Result = " do  {  tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } } while ( " + BOOLEAN_EXPRESSION() + ") ;";
            }

            Depth -= 1;

            return Result;
        }

        public static string FOR_STATEMENT(bool AddStatement)
        {
            string Result = string.Empty;

            Depth += 1;

            if (!AddStatement)
            {
                Result = " for (" + SIMPLE_ASSIGNMENT() + "; " + BOOLEAN_EXPRESSION() + ";" + INC_DEC() + ") { tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } " + "}";
            }
            else
            {
                Result = " for (" + SIMPLE_ASSIGNMENT() + "; " + BOOLEAN_EXPRESSION() + ";" + INC_DEC() + ") { tmp_counter = tmp_counter + 1; if (tmp_counter >= 0x1000) { break; } " + S() + "}";
            }

            Depth -= 1;

            return Result;
        }

        public static string SIMPLE_ASSIGNMENT()
        {
            var RandomNum = Rand.Next(5);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            switch (RandomNum)
            {
                case 0:

                    Result = L_VALUE() + " = " + EXPRESSION() + SIMPLE_ASSIGNMENTP();
                    Depth -= 1;

                    return Result;

                case 1:

                    Result = "";
                    Depth -= 1;

                    return Result;

                default:

                    Result = "val = 10";
                    Depth -= 1;

                    return Result;

            }


        }

        public static string SIMPLE_ASSIGNMENTP()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string INC_DEC()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(4);

            Depth += 1;

            Result = L_VALUE() + INC_DECP();

            Depth -= 1;

            return Result;
        }

        public static string INC_DECP()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = "--" + DECP();

            Depth -= 1;

            return Result;
        }

        public static string INCP()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string DECP()
        {
            string Result = string.Empty;

            return Result;
        }

        public static string BOOLEAN_EXPRESSION()
        {
            string Result = string.Empty;
            string Expr = string.Empty;
            string[] Operators = { " ", " == ", " <= ", " >= ", " <> ", " >< ", " ! ", " ; ", " != ", " = ", " > ", " < ", "((", "(", ")", "))", };
            var RandomNum = Rand.Next(0, Operators.Length);
            var RandomNum2 = Rand.Next(3);

            Depth += 1;

            Expr = EXPRESSION();

            if (RandomNum2 == 0)
            {
                //
                // Two sides are equal
                //
                Result = Expr + Operators[RandomNum] + Expr + SIMPLE_ASSIGNMENTP();
                Depth -= 1;

                return Result;
            }
            else
            {
                //
                // Two sides are not equal 
                //
                string Expr2 = EXPRESSION(true);

                if (Expr2.Length >= 150)
                {
                    Expr2 = EXPRESSION(true);
                }

                Result = Expr + Operators[RandomNum] + Expr2 + SIMPLE_ASSIGNMENTP();
                Depth -= 1;

                return Result;
            }
        }

        public static string EXPRESSION()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E1() + E0P();

            Depth -= 1;

            return Result;
        }

        public static string EXPRESSION(bool ForceToBeValid)
        {
            string Result = string.Empty;

            if (!ForceToBeValid)
            {
                return EXPRESSION();
            }
            else
            {
                while (true)
                {
                    string Expr = EXPRESSION();

                    if (HighLevelScriptGen.EvaluateExpression(Expr, ref Result))
                    {
                        return Expr;
                    }
                }

            }
        }

        public static string E0P()
        {
            var RandomNum = Rand.Next(2);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " | " + E1() + E0P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E1()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E2() + E1P();

            Depth -= 1;

            return Result;
        }

        public static string E1P()
        {
            var RandomNum = Rand.Next(2);
            string Result = string.Empty;

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " ^ " + E2() + E1P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E2()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E3() + E2P();

            Depth -= 1;

            return Result;
        }

        public static string E2P()
        {
            string Result = string.Empty;

            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " & " + E3() + E2P();
                Depth -= 1;
                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;
                return Result;
            }
        }

        public static string E3()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E4() + E3P();

            Depth -= 1;

            return Result;
        }

        public static string E3P()
        {
            string Result = string.Empty;

            var RandomNum = Rand.Next(2);

            Depth += 1;
            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " >> " + E4() + E3P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E4()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E5() + E4P();

            Depth -= 1;

            return Result;
        }

        public static string E4P()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " << " + E5() + E4P();
                Depth -= 1;
                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;
                return Result;
            }
        }

        public static string E5()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E6() + E5P();

            Depth -= 1;

            return Result;
        }

        public static string E5P()
        {
            string Result = string.Empty;

            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " + " + E6() + E5P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E6()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E7() + E6P();

            Depth -= 1;

            return Result;
        }

        public static string E6P()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " - " + E7() + E6P();
                Depth -= 1;
                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;
                return Result;
            }
        }

        public static string E7()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E8() + E7P();

            Depth -= 1;

            return Result;
        }

        public static string E7P()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " * " + E8() + E7P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E8()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E9() + E8P();

            Depth -= 1;

            return Result;
        }

        public static string E8P()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " / " + E9() + E8P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E9()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E10() + E9P();

            Depth -= 1;

            return Result;
        }

        public static string E9P()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(2);

            Depth += 1;

            if (Depth >= MAX_DEPTH)
            {
                RandomNum = 1;
            }

            if (RandomNum == 0)
            {
                Result = " % " + E10() + E9P();
                Depth -= 1;

                return Result;
            }
            else
            {
                Result = "";
                Depth -= 1;

                return Result;
            }
        }

        public static string E10()
        {
            string Result = string.Empty;

            Depth += 1;

            Result = E12();

            Depth -= 1;

            return Result;
        }

        public static string E13()
        {
            string Result = string.Empty;

            Result = "";

            return Result;
        }
        public static string L_VALUE()
        {
            string Result = string.Empty;

            var RandomNum = Rand.Next(2);

            if (RandomNum == 0)
            {
                Result = "val";  // id
                return Result;
            }
            else if (RandomNum == 1)
            {
                Result = "@rax"; // register
                return Result;
            }

            return string.Empty;
        }

        public static string NULL()
        {
            string Result = string.Empty;

            return Result;

        }

        public static string E12()
        {
            string Result = string.Empty;
            var RandomNum = Rand.Next(0, 8 + Identifiers.Count);
            var RandomNum2 = 0;

            Depth += 1;

            switch (RandomNum)
            {
                case 0:

                    Result = "(" + EXPRESSION() + ")";
                    Depth -= 1;

                    return Result;

                case 1:

                    RandomNum2 = Rand.Next(0, 20);
                    Result = "0x" + RandomNum2.ToString("X");  // hex
                    Depth -= 1;

                    return Result;

                case 2:

                    RandomNum2 = Rand.Next(0, 20);
                    Result = "0n" + RandomNum2.ToString(); // decimal
                    Depth -= 1;

                    return Result;

                case 3:

                    RandomNum2 = Rand.Next(0, 20);

                    //
                    // C# don't support prefix for octals
                    //
                    // Result = "0t" + Convert.ToString(RandomNum2, 8); // octal
                    Result = Convert.ToString(RandomNum2, 8); // octal
                    Depth -= 1;

                    return Result;

                case 4:

                    RandomNum2 = Rand.Next(0, 20);
                    Result = "0y" + Convert.ToString(RandomNum2, 2);  // binary
                    Depth -= 1;

                    return Result;

                case 5:

                    Result = "-" + E12() + E13();
                    Depth -= 1;

                    return Result;

                case 6:

                    Result = "+" + E12() + E13();
                    Depth -= 1;

                    return Result;

                case 7:

                    Result = "~" + E12() + E13();
                    Depth -= 1;

                    return Result;

                default:

                    RandomNum2 = Rand.Next(0, Identifiers.Count);
                    Result = Identifiers[RandomNum2].IdentifierName; // register, pesudo-registers, ids whatever
                    Depth -= 1;

                    return Result;
            }
        }
    }
}