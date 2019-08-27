import datetime
import os
import json
import subprocess
import threading
import time

class TimedRun:
    """A run of an external process, with a timeout.
    
    Call start() to begin running, then periodically call check()
    to detect when it is done or terminate it if it times out, e.g.:
    
        run = TimedRun(cmd, 1000)
        run.start()
        while True:
          time.sleep(1)
          if run.check() != TimedRun.RUNNING:
            break
    """
    
    COMPLETED = 1
    RUNNING = 0
    TIMEDOUT = 2
    
    def __init__(self, cmd, timeout=60, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL):
        """Constructor.
        
        Args:
        cmd -- command line spec, as passed to subprocess.Popen
        timeout -- timeout in seconds
        stdout -- 
        """
        self.cmd = cmd
        self.stdoutf = stdout
        self.stderrf = stderr
        
        self.timeout = timeout
        self.start_time = None
        self.end_time = None
        
        self.process = None
        self.thread = None
        
        self.result = None
        self.retcode = None
        self.stdout = None
        self.stderr = None
        
    def __del__(self):
        """Destructor."""
        if self.process is not None:
            self.process.terminate()
        if self.thread is not None:
            self.thread.join()
            
    def longname(self):
        """Returns the cmd, strung together."""
        return ' '.join(self.cmd)

    def tabname(self, limit=100):
        """Returns a table-appropriate representation of the run."""
        solver = os.path.basename(self.cmd[0])
        target = os.path.basename(self.cmd[1])[:40]
        outdir = os.path.dirname(self.cmd[2])[:40]
        args = ' '.join(self.cmd[3:])
        ret = '{} {:<40} -> {:<30} | {}'.format(solver, target, outdir, args)
        return ret[:limit]
            
    def elapsed(self):
        """Total running time so far (in seconds)."""
        if self.end_time is None:
            return time.time() - self.start_time
        else:
            return self.end_time - self.start_time
        
    def completed(self):
        """True iff the run completed without timing out."""
        return self.result == self.COMPLETED
       
    def start(self):
        """Start running the command."""
        self.start_time = time.time()
        self.thread = threading.Thread(target=self._run)
        self.thread.start()
        
    def stop(self):
        """Terminate an ongoing process, whether it's done or not."""
        self.process.terminate()
        self.thread.join()

    def check(self):
        """Check for completion/timeout state."""
        self.thread.join(0)
        if not self.thread.is_alive():
            self.result = self.COMPLETED
            return self.COMPLETED
        elif self.elapsed() > self.timeout:
            self.stop()
            self.result = self.TIMEDOUT
            return self.TIMEDOUT
        else:
            return self.RUNNING
        
    def to_dict(self):
        """Returns a jsonifiable dict of the main results."""
        ret = {}
        ret['command_line'] = self.cmd
        ret['elapsed_time'] = self.elapsed()
        ret['return_code'] = self.retcode
        if self.result == self.COMPLETED:
            ret['result'] = 'OK'
        else:
            ret['result'] = 'TIMEOUT'
        return ret
        
    def _run(self):
        """Thread worker function (do not call directly)."""
        self.start_time = time.time()
        self.process = subprocess.Popen(self.cmd, stdout=self.stdoutf, stderr=self.stderrf)
        self.process.wait()
        self.end_time = time.time()
        self.retcode = self.process.returncode


class TimedRunBatch:
    """A batch of timed runs, using a configurable number of threads."""
    
    def __init__(self, cmds, timeout=60, thread_count=1, poll_period=1.0, outfiles=None):
        """Constructor.
        
        Args:
        cmds -- list of command line specs
        timeout -- timeout in seconds (same for every process)
        thread_count -- max number of simultaneous runs
        poll_period -- how often to poll ongoing runs, in seconds
        """
        self.cmds = cmds
        self.outfiles = outfiles
        self.timeout = timeout
        self.thread_count = thread_count
        self.poll_period = poll_period
        
        self.done = []
        self.underway = []
    
    def run(self):
        """Run the batch."""
        while len(self.done) != len(self.cmds):
            # Check all current runs
            underway = []
            for run in self.underway:
                c = run.check()
                if c != TimedRun.RUNNING:
                    self.done.append(run)
                    self._on_done(run, c)
                else:
                    underway.append(run)
            self.underway = underway
            
            # Add new runs if there is room
            available = self.thread_count - len(self.underway)
            nextindex = len(self.done) + len(self.underway)
            unstarted = len(self.cmds) - nextindex
            startable = min(unstarted, available)
            
            for i in range(startable):
                index = nextindex + i
                cmd = self.cmds[index]
                if self.outfiles is not None:
                    outfile = open(self.outfiles[index], 'w')
                    run = TimedRun(cmd, self.timeout, outfile)
                else:
                    run = TimedRun(cmd, self.timeout)
                run.start()
                self.underway.append(run)
                self._on_start(run)
            
            # Have a nap
            time.sleep(self.poll_period)
            
    def to_dict(self):
        """Returns a jsonifiable dict."""
        ret = {}
        ret['timeout'] = self.timeout
        ret['runs'] = [run.to_dict() for run in self.done]
        return ret
    
    def to_json(self, filename):
        """Writes the results to JSON."""
        with open(filename, 'w') as f:
            ret = self.to_dict()
            json.dump(ret, f)
            
    def _on_start(self, run):
        """Called when a run is started."""
        pass
            
    def _on_done(self, run, result):
        """Called when a run completes."""
        when = datetime.datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        what = run.tabname(160)
        rslt = run.retcode if run.completed() else 'TIMEOUT'
        wait = run.elapsed()
        done = len(self.done)
        total = len(self.cmds)
        print('{} {:<120} {:>7} in {:8.2f} s. {:>3}/{:>3} done'.format(when, what, rslt, wait, done, total))


class TimedRunBatchPW(TimedRunBatch):
    """A timed run batch with partial writes."""

    def __init__(self, cmds, outfile, *args, **kwargs):
        """Constructor.
        
        Args:
        cmds -- list of command line specs
        outfile -- file where partial results are output
        args, kwargs -- passed to TimedRunBatch constructor
        """
        super().__init__(cmds, *args, **kwargs)
        self.partial_outfile = outfile

    def _on_done(self, run, result):
        """Called when a run completes."""
        super()._on_done(run, result)
        self.to_json(self.partial_outfile)

