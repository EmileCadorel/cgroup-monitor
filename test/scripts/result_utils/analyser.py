import yaml
import time
import json
from scipy.signal import savgol_filter
from scipy.signal import lfilter
import math
import numpy as np

# *************************************
# Class used to analyse the results of a scenario
# *************************************
class ResultAnalyser :

    # *************************************
    # Create a new empty analyser
    # @params:
    #    - client: the database client used to retreive the results
    # *************************************
    def __init__ (self, client, scenario) :
        self._client = client
        self._results = self._client.getResult (self._readScenario (scenario))
        self._usageCpuVMs = {}
        self._freqCpuVMs = {}
        self._cpuFreq = []
        self._rapl = []
        self._capCpuVMs = {}
        self._vmLegend = {}
        self._vcpus = {}
        self._alls = {}
        self._duration = []
    
    # *************************************
    # Analyse a scenario result
    # *************************************
    def run (self):
        self._readVMLegends ()
        self._readVMs ()
        self._analyseCpuMemResults ()
        
        print (self._latexHeader ())
        self.computeMeanResult ()
        self._computeMeanOutput ()
        print (sum (self._duration) / len (self._duration))
        variance = self._smooth (self.computeVariationCPUSpeed (), 101)
        print (sum (variance) / len (variance))
        
        print (self._plotCpuResult ())
        print (self._plotAverageCpu ())
        self._plotOutput ()
        print (self._latexFooter ())
        

    # *************************************
    # Read the type of scenario in order to run the correct analyse
    # For example, alloc_script and compress-7zip does not have the same result output
    # @params:
    #    - vmName: the name of the VM that runned the test
    # @returns: the type of test runned on vm "vmName" (ex: ("phoronix", "compress-7zip"))
    # *************************************    
    def _readScenarioType (self, vmName) : 
        if (len (self._results) != 0):
            content = yaml.load (self._results[0]["scenario"], Loader=yaml.FullLoader)
            for v in content["vms"] :
                if (v["name"] == vmName[:-1]):
                    if ("name" in v["test"]):
                        return (v["test"]["type"], v["test"]["name"])
                    else:
                        return (v["test"]["type"], "")

        return ("", "")
    
    # *************************************
    # Read the list of VMs in the results
    # *************************************    
    def _readVMs (self):
        if (len (self._results) != 0):
            for v in self._results[0]["placement"] :
                self._usageCpuVMs [v] = [[[] for i in range (len (self._results))] for vcpu in range (self._vcpus[v])]
                self._freqCpuVMs [v] = [[[] for i in range (len (self._results))] for vcpu in range (self._vcpus[v])]
                self._capCpuVMs [v] = [[[] for i in range (len (self._results))] for vcpu in range (self._vcpus[v])]

    # *************************************
    # Read data about VMs to create legend
    # *************************************    
    def _readVMLegends (self) :
        if (len (self._results) != 0):
            content = yaml.load (self._results [0]["scenario"], Loader=yaml.FullLoader)
            for v in content ["vms"] :
                for i in range (v["instances"]):
                    self._vmLegend [v["name"] + str (i)] = v["name"] + str (i) + " " + str (v["vcpus"]) + "@" + str (v["frequency"]) + "MHz, " + " of " + str (v["memory"]) + "MB";
                    self._vcpus [v["name"] + str (i)] = v["vcpus"]


                
    # *************************************
    # Read the cpu and mem results from the cpu market
    # *************************************
    def _analyseCpuMemResults (self) : 
        for i in range (len (self._results)) :
            for mon in self._results[i]["monitors"]:
                self._analyseCpuMemResultFile (self._results[i]["monitors"][mon], i)
        
    # *************************************
    # Analyse the cpu and mem result of a given file
    # *************************************
    def _analyseCpuMemResultFile (self, jfile, index) :
        period = 10000.0
        startInstant = None
        for line in jfile.splitlines () :
            j = json.loads (line)            
            if ('cpu-control' in j) : 
                for vm in j["cpu-control"] :
                    max_cpus =  1000000
                    for vcpu in range (self._vcpus[vm]) :
                        self._usageCpuVMs[vm][vcpu][index] = self._usageCpuVMs[vm][vcpu][index] + [j["cpu-control"][vm][vcpu]["cycles"] / period * 100.0]
                        self._freqCpuVMs[vm][vcpu][index] = self._freqCpuVMs[vm][vcpu][index] + [j["cpu-control"][vm][vcpu]["frequency"]]
                        self._capCpuVMs [vm][vcpu][index] = self._capCpuVMs[vm][vcpu][index] + [(j["cpu-control"][vm][vcpu]["capping"] * 100.0)]                               
            if ("freq" in j):
                if (len (self._cpuFreq) == 0):
                    self._cpuFreq = [[[] for i in range (len (self._results))] for cpu in range (len (j["freq"]))]
                    
                for i in range (len (j["freq"])) :
                    self._cpuFreq[i][index] = self._cpuFreq[i][index] + [j["freq"][i]]
            if ("rapl0" in j) :
                self._rapl = self._rapl + [j["rapl0"] / 1000000.0]
            self._duration = self._duration + [j["cpu-duration"]]

    def computeMeanResult (self):
        alls = {}
        nb_alls = {}
        for v in self._usageCpuVMs :
            for vcpu in range (self._vcpus[v]):
                res = self._freqCpuVMs[v][vcpu][0]
                name = self._vmLegend[v][0:3]
                if (name in alls) :
                    alls[name] = self.add (alls[name], res)
                    nb_alls[name] = nb_alls[name] + 1
                else :
                    alls[name] = res
                    nb_alls[name] = 1

        for i in nb_alls :
            for j in range (len (alls[i])):
                alls[i][j] = (alls[i][j] / nb_alls[i] / 1000) 
        self._alls = alls

    def computeVariationCPUSpeed (self):
        variance = []
        for i in range (len (self._cpuFreq[0][0])):
            mean = 0
            for cpu in range (len (self._cpuFreq)):
                mean = mean + (self._cpuFreq[cpu][0][i] / 1000)
            mean = mean / len (self._cpuFreq)
            SS = 0
            for cpu in range (len (self._cpuFreq)):
                x = ((self._cpuFreq[cpu][0][i]/1000) - mean)
                SS = SS + (x * x)
            variance = variance + [math.sqrt (SS / (len (self._cpuFreq) - 1))]
        return variance                
        
    def add (self, a, b):
        if (a == []) :
            return b
        else :
            res = []
            for i in range (len( a)):
                res = res + [a[i] + b[i]]
            return res
                    
    def _plotCpuResult (self):
        result = ""
        for i in range (len (self._results)) :
            result = result + "\section {Iteration " + str(i) + ", CPU results}\n"

            # for v in self._usageCpuVMs :
            #     result = result+ """
            #     \\begin{figure}[h]
            #     \centering
            #     \scalebox{1.2}{
            #     \\begin{tikzpicture}
            #     \\begin{axis} [ylabel=Speed in MHz, xlabel=time (s),
            #     legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
            #     smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]                               
            #     """
            #     for vcpu in range (self._vcpus[v]) :
            #         result = result + """
            #         \\addplot [mark=otimes] coordinates {
            #         """
            #         res = self._smooth (self._usageCpuVMs[v][vcpu][i], 101)
            #         for j in range (len (res)) :
            #             result = result + "\n(" + str (j) + ", " + str (res [j]) + ")"
                        
            #         result = result + """
            #         };
            #         \\addlegendentry{Frequency """+ str (vcpu) + """};
            #         """
            #     result = result + """
            #     \end{axis}
            #     \end{tikzpicture}     
            #     }   
            #     \caption{
            #     """ + self._vmLegend [v] + "}\n\end{figure}\n\n\n\pagebreak"

                
            # for cpu in range (len (self._cpuFreq)) :
            #     result = result + """
            #     \\begin{figure}[h]
            #     \centering
            #     \scalebox{1.2}{
            #     \\begin{tikzpicture}
            #     \\begin{axis} [ylabel=Speed in MHz, xlabel=time (s),
            #     legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
            #     smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]                               
            #     """

            #     result = result + """
            #     \\addplot [mark=otimes] coordinates {
            #     """

            #     res = self._smooth (self._cpuFreq[cpu][i], 101)
            #     for j in range (len (res)) :
            #         result = result + "\n(" + str (j) + ", " + str (res [j]) + ")"
                    
            #     result = result + """
            #     };
            #     \\addlegendentry{Frequency """+ str (cpu) + """};
            #     """
                    
            #     result = result + """
            #     \end{axis}
            #     \end{tikzpicture}     
            #     }   
            #     \caption{Host frequency}\n\end{figure}\n\n\n\pagebreak"""

        return result

    def _plotAverageCpu (self) :
        result = ""
        self._alls["rapl"] = self._rapl
        for v in self._alls :
            result = result+ """
            \\begin{figure}[h]
            \centering
            \scalebox{1.2}{
            \\begin{tikzpicture}
            \\begin{axis} [ylabel=Speed in MHz, xlabel=time (s),
            legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
            smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]                               
            """
            result = result + """
            \\addplot [mark=otimes] coordinates {
            """
            res = self._alls[v]
            for j in range (len (res)) :
                result = result + "\n(" + str (j) + ", " + str (res [j]) + ")"
                    
            result = result + """
            };
            \\addlegendentry{Frequency """+ str (v) + """};
            """
            result = result + """
            \end{axis}
            \end{tikzpicture}     
            }   
            \caption{Average 
            """ + v + "}\n\end{figure}\n\n\n\pagebreak"
        
        return result


    def _smooth (self, values, length) :
        n = 15
        b = [1.0 / n] * n
        w = lfilter (b, 1, values) #savgol_filter(values, length, 2)
        return w


    def _plotOutput (self):
        for i in range (len (self._results)) : 
            print ("\section {Iteration " + str(i) + ", VM output results}\n")
            for v in self._results[i]["vms"]:
                t = self._readScenarioType (v)
                if (t == ("custom", "alloc_script")):
                    self._plotAlloc (v, i)
                if (t == ("deathstar", "hotel")) :
                    self._plotHotel (v, i)
                if (t == ("phoronix", "compress-7zip")) :
                    self._plotCompress (v, i)

                
    def _plotAlloc (self, v, it):
        times = []
        mem = []
        used = []
        swap = []        
        for line in self._results[it]["vms"][v].splitlines ():
            if (line[:4] == "Done"):
                times = times + [float (line[7:-2])]
            elif (line[:4] == "Mem:") :
                s = line.split ()
                mem = mem + [int (s[1])]
                used = used + [int(s[2])]
            elif (line[:5] == "Swap:") :
                s = line.split ()
                swap = swap + [int(s[2])]
        
        result = """
        \\begin{figure}[h]
        \centering
        \scalebox{1.2}{
        \\begin{tikzpicture}
        \\begin{axis} [ylabel=Usage in MB, xlabel=time (s),
        legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
        smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
        \\addplot [mark=otimes, color=blue!50] coordinates {
        """
        for j in range (len (used)) :
            result = result + "\n(" + str (j) + ", " + str (used[j]) + ")"
                
        result = result + """
        };
        \\addlegendentry{Used memory}
        \\addplot [mark=otimes, color=green!40!gray] coordinates {
        """
                
        for j in range (len (mem)) :
            result = result + "\n(" + str (j) + ", " + str (mem [j]) + ")"
            
        result = result + """
        };
        \\addlegendentry{Usable Memory}
        \\addplot [mark=otimes] coordinates {
        """
        for j in range (len (swap)) :
            result = result + "\n(" + str (j) + ", " + str (swap[j]) + ")"
            
        result = result + """
        };
        \\addlegendentry{Used swap}
        \end{axis}
        \end{tikzpicture}        
        }
        \caption{
        """ + self._vmLegend [v] + "}\n\end{figure}\n\n\n\\pagebreak"

        result = result + """
        \\begin{figure}[h]
        \centering
        \scalebox{1.2}{
        \\begin{tikzpicture}
        \\begin{axis} [ylabel=time (s), xlabel=time (s),
        legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
        smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
        \\addplot [mark=otimes, color=blue!50] coordinates {
        """
        for j in range (len (times)) :
            result = result + "\n(" + str (j) + ", " + str (times[j]) + ")"
            
        result = result + """
        };
        \\addlegendentry{Memory access speed}
        \end{axis}
        \end{tikzpicture}        
        }
        \caption{
        """ + self._vmLegend [v] + "}\n\end{figure}\n\n\n\\pagebreak"
        print (result)



    def _plotHotel (self, v, it) :
        latency = []
        latency_y = []
        lines = self._results[it]["vms"][v].splitlines ()
        i = 0
        for line in lines :
            print (line)
            if ("Latency Distribution (HdrHistogram - Recorded Latency)" in line) :
                for j in lines[i+1:i+9]:
                    latency_y = latency_y + [j[0:7]]
                    if (j[-2] == 'u') : 
                        latency = latency + [j[9:-2]]
                    elif j[-2] == 'm':
                        latency = latency + [float (j[9:-2]) * 1000]
                    else :
                        latency = latency + [float (j[9:-2]) * 1000000]            
            i = i + 1


        result = """
        \\begin{figure}[h]
        \centering
        \scalebox{1.2}{
        \\begin{tikzpicture}
        \\begin{axis} [ylabel=Latency (milliseconds), xlabel=Percentile,
        legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
        smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
        \\addplot [mark=otimes, color=blue!50] coordinates {
        """
        for j in range (len (latency)) :
            result = result + "\n(" + str (latency_y[j]) + ", " + str (latency [j]) + ")"
                
        result = result + """
        };
        \end{axis}
        \end{tikzpicture}        
        }
        \caption{
        """ + self._vmLegend [v] + "}\n\end{figure}\n\n\n\\pagebreak"
        
        print (result)

    def _computeMeanOutput (self):
        done = False
        compression = {}
        decompression = {}
        nb = {}
        for v in self._results[0]["vms"]:            
            t = self._readScenarioType (v)
            if (t == ("phoronix", "compress-7zip")) :
                (c, d) = self._getCompressOutput (v, 0)
                if v[0:3] not in compression : 
                    compression [v[0:3]]= c                
                    decompression [v[0:3]]= d
                    nb [v[0:3]] = 1
                else :
                    compression [v[0:3]] = self._add (compression[v[0:3]], c)
                    decompression [v[0:3]] = self._add (decompression[v[0:3]], d)
                    nb [v[0:3]] = nb [v[0:3]] + 1
                    
        for i in compression:
            for j in range (len (compression[i])):
                compression[i][j] = compression[i][j] / nb[i]
        for i in decompression:
            for j in range (len (decompression[i])):
                decompression[i][j] = decompression[i][j] / nb[i]

        print (compression)
        print (decompression)

    def _add (self, a, b):
        print (len (b))
        res = a
        for i in range (min (len (a), len (b))):
            res [i] = int(a[i]) + int (b[i])
        return res

    def _getCompressOutput (self, v, it) :
        compression = []
        decompression = []
        lines = self._results[it]["vms"][v].splitlines ()
        i = 0
        for line in lines : 
            if ("Test: Compression Rating:" in line):
                for j in lines[i+1:]:
                    if j == "":
                        break
                    else :
                        compression = compression + [int (j.split ()[-1].replace ("\x1b[1;32m", "").replace ("\x1b[0m", "").replace ("\x1b[1;31m", ""))]                            
            if ("Test: Decompression Rating:" in line):
                for j in lines[i+1:]:
                    if j == "":
                        break
                    else :
                        decompression = decompression + [int (j.split ()[-1].replace ("\x1b[1;32m", "").replace ("\x1b[0m", "").replace ("\x1b[1;31m", ""))]
            i = i + 1
        return (compression, decompression)
        

    def _plotCompress (self, v, it) :
        (compression, decompression) = self._getCompressOutput (v, it)
        result = """
        \\begin{figure}[h]
        \centering
        \scalebox{1.2}{
        \\begin{tikzpicture}
        \\begin{axis} [ylabel=Rate, xlabel=Run,
        legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
        smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
        \\addplot [mark=otimes, color=blue!50] coordinates {
        """
        for j in range (len (compression)) :
            result = result + "\n(" + str (j) + ", " + str (compression [j]) + ")"
                
        result = result + """
        };

        \\addplot [mark=otimes, color=green!50] coordinates {
        """
        for j in range (len (decompression)) :
            result = result + "\n(" + str (j) + ", " + str (decompression [j]) + ")"                
        result = result + """
        };

        \end{axis}
        \end{tikzpicture}        
        }
        \caption{
        """ + self._vmLegend [v] + "}\n\end{figure}\n\n\n\\pagebreak"
        
        print (result)

            

    def _latexHeader (self) :
        return """\documentclass[paper=a4,
        fontsize=11pt,
        ]{article}        

        \\usepackage{xspace}
        \\usepackage{amsmath,amssymb,amsfonts}
        \\usepackage{rotating}
        \\usepackage{multicol}

        \\usepackage{graphicx}
        \\usepackage{textcomp}
        \\usepackage{xcolor,colortbl}
        \\usepackage{url}
        \\usepackage{hyperref}
        \\usepackage[utf8]{inputenc}
        \\usepackage{tikz}
        \\usepackage{pgfplots}
        \\usepackage{mathtools, amsmath, relsize}
        \\usepgfplotslibrary{external} 
        \\tikzexternalize
        \\begin{document}
        """
    

    def _latexFooter (self) :
        return """
        \end{document}
        """    
    
    
    # *************************************
    # Read the file containing the scenario and returns it into a string
    # @params:
    #    - scenario: the path of the scenario file
    # @returns: the content of the scenario in a string
    # *************************************
    def _readScenario (self, scenario) :
        with open (scenario, "r") as fp :
            content = yaml.load (fp.read (), Loader=yaml.FullLoader)
            s = yaml.dump (content)
            return s


    
