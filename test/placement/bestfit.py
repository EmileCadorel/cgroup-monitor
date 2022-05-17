def bestFit(blockSize, m, processSize, n):
    allocation = [-1] * n
    small = [0 for i in range (len (blockSize))]
    medium = [0 for i in range (len (blockSize))]
    large = [0 for i in range (len (blockSize))]
    all = [0 for i in range (len (blockSize))]
	
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
            if (i < 250):
                small[bestIdx] = small[bestIdx] + 1
            elif i < 300 :
                medium[bestIdx] = medium[bestIdx] + 1
            else :
                large[bestIdx] = large[bestIdx] + 1
            all [bestIdx] += 1

    # print("Process No. Process Size	 Block no.")
    # for i in range(n):
    #     print(i + 1, "		 ", processSize[i],
    #           end = "		 ")
    #     if allocation[i] != -1:
    #         print(allocation[i] + 1)
    #     else:
    #         print("Not Allocated")
            
    # print (blockSize)
    # print (small)
    # print (medium)
    print (large)
    print (all)
    
if __name__ == '__main__':
    blockSize = [960 for i in range (12)] + [1536 for i in range (10)]
    processSize = [20 for i in range (250)] + [48 for i in range (50)] + [72 for i in range (100)]

    m = len(blockSize)
    n = len(processSize)

    bestFit(blockSize, m, processSize, n)
    
    blockSize = [40 for i in range (12)] + [64 for i in range (10)]
    processSize = [2 for i in range (250)] + [4 for i in range (50)] + [4 for i in range (100)]

    m = len(blockSize)
    n = len(processSize)

    bestFit(blockSize, m, processSize, n)

    blockSize = [int (40 * 1.8) for i in range (12)] + [int (64 * 1.8) for i in range (10)]
    processSize = [2 for i in range (250)] + [4 for i in range (50)] + [4 for i in range (100)]

    m = len(blockSize)
    n = len(processSize)

    bestFit(blockSize, m, processSize, n)
	
