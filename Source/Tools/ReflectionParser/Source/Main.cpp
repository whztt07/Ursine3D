#include "Precompiled.h"

#include "ReflectionOptions.h"
#include "ReflectionParser.h"

#include "Switches.h"

void parse(const po::variables_map &cmdLine);

int main(int argc, char *argv[])
{
    // misc initialization
    {
        // path to the executable
        auto exeDir = fs::path( argv[ 0 ] );

        // set the working directory to the executable directory
        fs::current_path( exeDir.parent_path( ) );
    }

    // parse command line
    try 
    {
        po::options_description program( "Ursine3D Reflection Parser" );

        program.add_options( )
        ( 
            SWITCH_OPTION( Help ), 
            "Displays help information." 
        )
        ( 
            SWITCH_OPTION( TargetName ), 
            po::value<std::string>( )->required( ),
            "Input target project name." 
        )
        ( 
            SWITCH_OPTION( InputSource ), 
            po::value<std::string>( )->required( ), 
            "Source file (header) to compile reflection data from." 
        )
        ( 
            SWITCH_OPTION( OutputHeader ), 
            po::value<std::string>( )->required( ), 
            "Output generated C++ header file." 
        )
        ( 
            SWITCH_OPTION( OutputSource ), 
            po::value<std::string>( )->required( ), 
            "Output generated C++ source file." 
        )
        ( 
            SWITCH_OPTION( HeaderTemplate ), 
            po::value<std::string>( )->required( )->default_value( "Templates/Header.mustache" ), 
            "Optional C++ header template (mustache) file." 
        )
        ( 
            SWITCH_OPTION( SourceTemplate ), 
            po::value<std::string>( )->required( )->default_value( "Templates/Source.mustache" ), 
            "Optional C++ source template (mustache) file" 
        )
        ( 
            SWITCH_OPTION( CompilerFlag ), 
            po::value<std::vector<std::string>>( )->multitoken( ), 
            "Optional list of flags to pass to the compiler." 
        );

        po::variables_map cmdLine;

        po::store( po::parse_command_line( argc, argv, program ), cmdLine );

        if (cmdLine.count( "help" )) 
        {
            std::cout << program << std::endl;

            return EXIT_SUCCESS;
        }

        po::notify( cmdLine );

        parse( cmdLine );
    }
    catch (std::exception &e) 
    {
        utils::FatalError( e.what( ) );
    }
    catch (...) 
    {
        utils::FatalError( "Unhandled exception occurred!" );
    }

    return EXIT_SUCCESS;
}

void parse(const po::variables_map &cmdLine)
{
    ReflectionOptions options;

    options.targetName = cmdLine.at( kSwitchTargetName ).as<std::string>( );
    options.inputSourceFile = cmdLine.at( kSwitchInputSource ).as<std::string>( );

    options.outputHeaderFile = cmdLine.at( kSwitchOutputHeader ).as<std::string>( );
    options.outputSourceFile = cmdLine.at( kSwitchOutputSource ).as<std::string>( );

    // default arguments
    options.arguments =
    { {
        "-x",
        "c++",
        "-std=c++11",
        "-DREFLECTION_PARSER"
    } };

    if (cmdLine.count( kSwitchCompilerFlag ))
    {
        auto flags = cmdLine.at( kSwitchCompilerFlag ).as<std::vector<std::string>>( );

        for (auto flag : flags)
            options.arguments.emplace_back( flag.c_str( ) );
    }

    auto tmplHeaderFile = cmdLine.at( kSwitchHeaderTemplate ).as<std::string>( );
    auto tmplSourceFile = cmdLine.at( kSwitchSourceTemplate ).as<std::string>( );
    
    ReflectionParser parser( options );

    parser.Parse( );

    // header template
    try
    {
        auto tmplHeader = utils::LoadText( tmplHeaderFile );

        utils::WriteText( options.outputHeaderFile, parser.GenerateHeader( *tmplHeader ) );
    } 
    catch (std::exception &e)
    {
        utils::FatalError( e.what( ) );
    }

    // source template
    try
    {
        auto tmplSource = utils::LoadText( tmplSourceFile );

        utils::WriteText( options.outputSourceFile, parser.GenerateSource( *tmplSource ) );
    } 
    catch (std::exception &e)
    {
        utils::FatalError( e.what( ) );
    }
}