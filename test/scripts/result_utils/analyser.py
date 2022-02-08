import yaml
import time
import json

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
        self._hostMemVMs = {}
        self._relUsageCpuVMs = {}
        self._guestMemVMs = {}
        self._capCpuVMs = {}
        self._allocMemVMs = {}
        self._vmLegend = {}
        self._vcpus = {}
    
    # *************************************
    # Analyse a scenario result
    # *************************************
    def run (self):
        self._readVMs ()
        self._readVMLegends ()
        self._analyseCpuMemResults ()

        print (self._latexHeader ())
        print (self._plotCpuResult ())
        print (self._plotMemResult ())
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
            for v in self._results[0]["scenario"]["vms"] :
                if (v["name"] == vmName[:-1]):
                    return (v["test"]["type"], v["test"]["name"])

        return ("", "")
    
    # *************************************
    # Read the list of VMs in the results
    # *************************************    
    def _readVMs (self):
        if (len (self._results) != 0):
            for v in self._results[0]["placement"] :
                self._usageCpuVMs [v] = [[] for i in range (len (self._results))]
                self._hostMemVMs [v] = [[] for i in range (len (self._results))]
                self._relUsageCpuVMs [v] = [[] for i in range (len (self._results))]
                self._guestMemVMs [v] = [[] for i in range (len (self._results))]
                self._capCpuVMs [v] = [[] for i in range (len (self._results))]
                self._allocMemVMs [v] = [[] for i in range (len (self._results))]

    # *************************************
    # Read data about VMs to create legend
    # *************************************    
    def _readVMLegends (self) :
        if (len (self._results) != 0):
            content = yaml.load (self._results [0]["scenario"], Loader=yaml.FullLoader)
            for v in content ["vms"] :
                self._vmLegend [v["name"]] = v["name"] + " " + str (v["vcpus"]) + "@" + str (v["frequency"]) + "MHz, " + str (v["memorySLA"]) + " of " + str (v["memory"]) + "MB";
                self._vcpus [v["name"]] = v["vcpus"]


                
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
        startInstant = None
        for line in jfile.splitlines () :
            j = json.loads (line)            
            if ('cpu-control' in j) : 
                for vm in j["cpu-control"] :
                    max_cpus = self._vcpus[vm[:-1]] * 1000000
                    self._usageCpuVMs[vm][index] = self._usageCpuVMs[vm][index] + [j["cpu-control"][vm]["host-usage"]]
                    self._relUsageCpuVMs[vm][index] = self._relUsageCpuVMs[vm][index] + [j["cpu-control"][vm]["relative-usage"]]
                    self._capCpuVMs [vm][index] = self._capCpuVMs[vm][index] + [(j["cpu-control"][vm]["capping"] / j["cpu-control"][vm]["period"] * 1000000.0) / max_cpus * 100.0]                               
            elif ('mem-control' in j) :
                for vm in j["mem-control"] :
                    self._hostMemVMs [vm][index] = self._hostMemVMs[vm][index] + [j["mem-control"][vm]["host-usage"]]
                    self._guestMemVMs [vm][index] = self._guestMemVMs[vm][index] + [j["mem-control"][vm]["guest-usage"]]
                    self._allocMemVMs [vm][index] = self._allocMemVMs[vm][index] + [j["mem-control"][vm]["allocated"]]



    def _plotCpuResult (self):
        result = ""
        for i in range (len (self._results)) :
            result = result + "\section {Iteration " + str(i) + ", CPU results}\n"

            for v in self._usageCpuVMs :
                result = result+ """
                \\begin{figure}[h]
                \centering
                \scalebox{0.7}{
                \\begin{tikzpicture}
                \\begin{axis} [ylabel=Usage in \%, xlabel=time (s),
                legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
                smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
                \\addplot [mark=otimes, color=blue!50] coordinates {
                """
                for j in range (len (self._usageCpuVMs[v][i])) :
                    result = result + "\n(" + str (j) + ", " + str (self._usageCpuVMs[v][i][j]) + ")"

                result = result + """
                };
                \\addlegendentry{Host usage}
                \\addplot [mark=otimes, color=green!40!gray] coordinates {
                """
                
                for j in range (len (self._capCpuVMs[v][i])) :
                    result = result + "\n(" + str (j) + ", " + str (self._capCpuVMs[v][i][j]) + ")"

                result = result + """
                };
                \\addlegendentry{Capping}
                \end{axis}
                \end{tikzpicture}     
                }   
                \caption{
                """ + self._vmLegend [v[:-1]] + "}\n\end{figure}\n\n\n"
                
        return result


    def _plotMemResult (self):
        result = ""
        for i in range (len (self._results)) :
            result = result + "\section {Iteration " + str(i) + ", Memory results}\n"

            for v in self._hostMemVMs :
                result = result+ """
                \\begin{figure}[h]
                \centering
                \scalebox{0.7}{
                \\begin{tikzpicture}
                \\begin{axis} [ylabel=Usage in MB, xlabel=time (s),
                legend style={nodes={scale=0.5, transform shape}, anchor=north west, draw=black, fill=white, align=left},
                smooth, mark size=0pt, cycle list name=exotic,  axis lines*=left]
                \\addplot [mark=otimes, color=blue!50] coordinates {
                """
                for j in range (len (self._hostMemVMs[v][i])) :
                    result = result + "\n(" + str (j) + ", " + str (self._hostMemVMs[v][i][j] / 1000) + ")"

                result = result + """
                };
                \\addlegendentry{Host usage}
                \\addplot [mark=otimes, color=green!40!gray] coordinates {
                """
                
                for j in range (len (self._guestMemVMs[v][i])) :
                    result = result + "\n(" + str (j) + ", " + str (self._guestMemVMs[v][i][j] / 1000) + ")"

                result = result + """
                };
                \\addlegendentry{Guest usage}
                \\addplot [mark=otimes] coordinates {
                """
                for j in range (len (self._allocMemVMs[v][i])) :
                    result = result + "\n(" + str (j) + ", " + str (self._allocMemVMs[v][i][j] / 1000) + ")"

                result = result + """
                };
                \\addlegendentry{Allocated}
                \end{axis}
                \end{tikzpicture}        
                }
                \caption{
                """ + self._vmLegend [v[:-1]] + "}\n\end{figure}\n\n\n"
                
        return result
                

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


    
