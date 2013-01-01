
package org.umd.periCSL;

import org.umd.periCSL.*;
import org.antlr.runtime.*;
import org.antlr.runtime.tree.*;
import org.antlr.stringtemplate.*;
import java.io.*;

public class Translate
{
    public static void main(String[] args) throws Exception
    {
        /*
          Upto three command line parameters :
          <CSL file>
          <template file>
          <output file>
        */

        int i=0;
        String inputFile = null;
        String templateFile = "periToPython.stg";
        String outputFile = "out";
        String arg;
        int done = 1;

        // check the arguments
        if(args.length <= 1)
        {
            done = 0;
        } else
        {
            while (i < args.length && args[i].startsWith("-"))
            {
                arg = args[i++];
                // check for -csl
                if(arg.equals("-csl"))
                {
                    if(i < args.length)
                    {
                        if(args[i].startsWith("-"))
                        {
                            System.out.println("-csl " + "must be followed by a filename");
                            done= 0;
                        }
                        else
                        {
                            inputFile = args[i++];
                        }
                    }
                    else
                    {
                        usage();
                        done = 0;
                    }
                }
                else if(arg.equals("-template"))
                {
                    if(i < args.length)
                    {
                        if(args[i].startsWith("-"))
                        {
                            System.out.println("-template " + "must be followed by a filename");
                            done= 0;
                        }
                        templateFile = args[i++];
                    }
                    else
                    {
                        done = 0;
                    }
                }
                else if(arg.equals("-out"))
                {
                    if(i < args.length)
                    {
                        if(args[i].startsWith("-"))
                        {
                            System.out.println("-out " + "must be followed by a filename");
                            done= 0;
                        }
                        outputFile = args[i++];
                    }
                    else
                    {
                        done = 0;
                    }
                }
            }
        }

        if(done == 0 && inputFile != null)
        {
            usage();
            System.exit(0);
        }

        // set up the template group
        FileReader groupFileR = new FileReader(templateFile);
        StringTemplateGroup templates =
            new StringTemplateGroup(groupFileR);
        groupFileR.close();


        // open the CSL file and read the search space description
        Reader reader = new FileReader(inputFile);
        PeriCSLLexer lexer = new PeriCSLLexer(new ANTLRReaderStream(reader));

        // create a buffer of tokens pulled from the lexer
        CommonTokenStream tokens = new CommonTokenStream(lexer);
        // create the parser
        PeriCSLParser parser = new PeriCSLParser(tokens);
        // parser's start rule is prog
        PeriCSLParser.prog_return r = parser.prog();

        // get the tree from the return structure for rule prog
        CommonTree t = (CommonTree)r.getTree();

        // create a stream of tree nodes from AST built by parser
        CommonTreeNodeStream nodes = new CommonTreeNodeStream(t);

        // tell it where it can find the token objects
        nodes.setTokenStream(tokens);

        // create the tree walker
        PeriCSLWalker walker = new PeriCSLWalker(nodes);
        walker.setTemplateLib(templates);

        PeriCSLWalker.prog_return r2 = walker.prog(parser.params, parser.region_sets,
                                                   parser.constants, parser.constraints,
                                                   parser.specification);

        // write the output
        BufferedWriter out = new BufferedWriter(new FileWriter(outputFile));
        out.write((r2.getTemplate()).toString());
        out.close();
        System.out.println((r2.getTemplate()).toString());
    }


    static void usage()
    {
        System.err.println("Usage: java org.umd.periCSL.Translate -csl <filename> -template <filename> -out <filename>");
        System.err.println("     -csl <filename> is a required argument. The other two are optional. ");
        System.err.println("     If no -template argument is provided, the target language defaults to Python.");
        System.err.println("     If no -out argument is provided, the output is written to 'out' file.");
    }

}
 
