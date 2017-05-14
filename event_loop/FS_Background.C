/*
 * Copyright (c) 2015
 *	Side Effects Software Inc.  All rights reserved.
 *
 * Redistribution and use of Houdini Development Kit samples in source and
 * binary forms, with or without modification, are permitted provided that the
 * following conditions are met:
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. The name of Side Effects Software may not be used to endorse or
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY SIDE EFFECTS SOFTWARE `AS IS' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 * NO EVENT SHALL SIDE EFFECTS SOFTWARE BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *----------------------------------------------------------------------------
 */

#include <UT/UT_DSOVersion.h>
#include <UT/UT_WritePipe.h>
#include <FS/FS_EventGenerator.h>
#include <CMD/CMD_Args.h>
#include <CMD/CMD_Manager.h>

//#define DEMO_DEBUG

namespace HDK_Sample {

/// @file
/// This example uses UT_WritePipe to open a pipe to an external process, then
/// use FS_EventGenerator to poll the list of running background tasks.
/// @see UT_WritePipe, FS_EventGenerator, CMD_Manager, CMD_Args

/// BackgroundTask is the object which keeps track of the background process
class BackgroundTask {
public:
     BackgroundTask() : myPipe(NULL) {}
    ~BackgroundTask() { close(); }

    /// Open a pipe to an external application
    /// @param cmd The command to open
    /// @param text The text to write to the pipe
    /// @param length The length of the @c text
    FILE		*open(const char *cmd, char *text=0, int length=0)
			 {
			     FILE	*fp;

			     myCommand.harden(cmd);
			     if (myPipe)
				close();	// Close existing pipe
			     myPipe = new UT_WritePipe();
			     fp = myPipe->open(cmd);
			     if (fp && text && length > 0)
			     {
				 // Due to the way operating systems work, the
				 // pipe may block at this point -- until the
				 // external application reads the data on the
				 // pipe that is.
				 if (fwrite(text, 1, length, fp) != length)
				 {
				     close();
				     fp = NULL;
				 }
				 else
				     fflush(fp);
			     }
			     return fp;
			 }

    /// Close the pipe, blocking until the pipe is complete
    void		close()
			{
			    if (myPipe)
			    {
				myPipe->close(true);
				delete myPipe;
				myPipe = NULL;
			    }
			}

    /// Test if the background process is complete
    bool		isComplete()
			{
			    return !myPipe || myPipe->isComplete(false);
			}

    UT_String		 myCommand;
    UT_WritePipe	*myPipe;
};

static UT_ValArray<BackgroundTask *>	theTasks;

static void
pollTasks()
{
    int		i;
#if defined(DEMO_DEBUG)
    fprintf(stderr, "Poll: %d\n", theTasks.entries());
#endif
    for (i = 0; i < theTasks.entries(); ++i)
    {
	if (theTasks(i)->isComplete())
	{
#if defined(DEMO_DEBUG)
	    if (theTasks(i)->myPipe)
		fprintf(stderr, "Delete [%d] %s\n",
				theTasks(i)->myPipe->getPid(),
				(const char *)theTasks(i)->myCommand);
#endif
	    delete theTasks(i);
	    theTasks(i) = 0;
	}
    }
    theTasks.collapse();
}

/// Function to list background tasks to a stream
static void
listTasks(std::ostream &os)
{
    int		i;
    os << theTasks.entries() << " background tasks\n";
    for (i = 0; i < theTasks.entries(); i++)
    {
	if (theTasks(i)->myPipe)
	{
	    os  << i
		<< " pid[" << theTasks(i)->myPipe->getPid()
		<< "] " << theTasks(i)->myCommand
		<< "\n";
	}
    }
}

/// @brief Event generator to poll for completed tasks
///
/// When Houdini has an opportunity, it will call this class to process any
/// events.  Ideally, we would use the FS_EventGenerator::getFileDescriptor()
/// method to return a file descriptor which allows a select() call to be used
/// rather than a polling process.  However, in this case, we need to poll the
/// background processes.
class BackgroundTimer : public FS_EventGenerator {
public:
	     BackgroundTimer() {}
    virtual ~BackgroundTimer() {}

    virtual const char	*getClassName() const	{ return "BackgroundTimer"; }

    /// Poll time is in ms, so poll every 0.5 seconds
    virtual int		 getPollTime() { return 500; }

    /// This callback is used to process events
    virtual int		 processEvents()
			 {
			     pollTasks();
			     // When no more tasks, stop polling
			     if (!theTasks.entries())
				 stopPolling();
			     // Return true if we want to be called again
			     return theTasks.entries() > 0;
			 }

    /// Stop polling process
    static void		stopPolling()
			{
#if defined(DEMO_DEBUG)
			    fprintf(stderr, "Stop polling\n");
#endif
			    if (theTimer)
			    {
				// Stop events from being sent to a deleted
				// object
				theTimer->uninstallGenerator();
				delete theTimer;
				theTimer = NULL;
			    }
			}
    /// Start polling process
    static void		startPolling()
			{
			    // Only create a timer if there isn't one already
			    if (!theTimer)
			    {
#if defined(DEMO_DEBUG)
				fprintf(stderr, "Start polling\n");
#endif
				theTimer = new BackgroundTimer();

				// Start the polling process
				if (!theTimer->installGenerator())
				{
				    delete theTimer;
				    theTimer = NULL;
				}
			    }
			}
private:
    static BackgroundTimer	*theTimer;
};

BackgroundTimer	*BackgroundTimer::theTimer = NULL;

// Command manager callback to handle the @c demo_background command
static void
hdk_background(CMD_Args &args)
{
    if (args.found('l'))
	listTasks(args.out());
    else if (args.argc() != 2)
    {
	args.err() << "Usage: " << args(0) << " [-l] 'command'\n";
	args.err() << "Runs a command in the background\n";
	args.err() << "The -l option lists all current commands\n";
    }
    else
    {
	BackgroundTask	*task;
	task = new BackgroundTask();
	if (task->open(args(1)))
	{
	    theTasks.append(task);
	    BackgroundTimer::startPolling();
	}
	else
	{
	    args.err() << "Error running: " << args(1) << "\n";
	}
    }
}

}	// end HDK Namespace

/// CMDextendLibrary is the plug-in hook called to install extensions to the
/// CMD library.
void
CMDextendLibrary(CMD_Manager *cman)
{
    // Install a new command
    cman->installCommand("hdk_background", "l", HDK_Sample::hdk_background);
}
