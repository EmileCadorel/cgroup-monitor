def bestFit(blockSize, m, processSize, n):
    allocation = [-1] * n
	
    for i in range(n):		
        bestIdx = -1
        for j in range(m):
            if blockSize[j] >= processSize[i]:
                if bestIdx == -1:
                    bestIdx = j
                elif blockSize[bestIdx] > blockSize[j]:
                    bestIdx = j

        if bestIdx != -1:			
            allocation[i] = bestIdx
            blockSize[bestIdx] -= processSize[i]

    print("Process No. Process Size	 Block no.")
    for i in range(n):
        print(i + 1, "		 ", processSize[i],
              end = "		 ")
        if allocation[i] != -1:
            print(allocation[i] + 1)
        else:
            print("Not Allocated")
            
    print (blockSize)
    
if __name__ == '__main__':
    # blockSize = [960 for i in range (12)] + [1536 for i in range (10)]
    # processSize = [20 for i in range (500)] + [72 for i in range (100)]

    blockSize = [40 for i in range (12)] + [64 for i in range (10)]
    processSize = [2 for i in range (350)] + [4 for i in range (100)]
    m = len(blockSize)
    n = len(processSize)

    bestFit(blockSize, m, processSize, n)
	
