from PIL import Image
import Queue
import math
huff = {}
colors = [(0, 0, 0),(127, 127, 127),(127, 0, 0),(127, 32, 0),(127, 127, 0),
(32 , 127, 0),(0, 127, 0),(0, 127, 32),(0, 127, 127),(0, 32, 127),(0, 0, 127),
(32, 0, 100),(88, 0, 127),(127, 0, 127),(127, 0, 88),(127, 0, 32)];
def closest_color(r, g, b):
	if (r>127 or g>127 or b>127):
		r = r/2
		g = g/2
		b = b/2
	closest = 999999
	num = -1
	for i in range(0,len(colors)):
		#print i,": ",(r, g, b) ,"-", (colors[i][0],colors[i][1],colors[i][2])
		dist = (((r-colors[i][0])**2)+((g-colors[i][1])**2)+((b-colors[i][2])**2))**0.5
		#print dist
		if dist < closest:
			closest = dist
			num = i
	#print "\nclosest:",closest," num:",num,"\n"
	return num
class HuffmanNode(object):
    def __init__(self,left=None,right=None,root=None):
        self.left = left
        self.right = right
        self.root = root
    def children(self):
        return (self.left,self.right)
def create_tree(frequencies):
    p = Queue.PriorityQueue()
    for value in frequencies:    # 1. Create a leaf node for each symbol
        p.put(value)             #    and add it to the priority queue
    while p.qsize() > 1:         # 2. While there is more than one node
        l, r = p.get(), p.get()  # 2a. remove two highest nodes
        node = HuffmanNode(l, r) # 2b. create internal node with children
        p.put((l[0]+r[0], node)) # 2c. add new node to queue      
    return p.get()               # 3. tree is complete - return root node

im = Image.open('out.png')
px = im.load()
size = im.size
mapping = {}
for y in range(0, size[1]):
	for x in range(0, size[0]):
		pix = px[x,y]
		if not(mapping.has_key(pix)):
			mapping[pix] = 1
		else:
			mapping[pix]= mapping[pix]+1
print mapping
fix_image_arr = []
count = 1
for key in mapping:
	#print key
	fix_image_arr.append([colors[closest_color(key[0],key[1],key[2])], key])
print mapping
#for key in mapping:
#	for key_2 in mapping:
#		c1 = closest_color(key[0], key[1], key[2])
#		c2 = closest_color(key_2[0], key_2[1], key_2[2])
#		#dist = (((key_2[0]-key[0])**2)+((key_2[1]-key[1])**2)+((key_2[2]-key[2])**2))**0.5
#		#if dist< 100 and dist>0:
#		if c1 == c2:
#			count+=1
#			if count%2 ==0:	
#				print "distance between"+str(key)+" and "+str(key_2)
#				print "probably the same"
#				if (mapping[key]>mapping[key_2]):
#					fix_image_arr.append([key, key_2])
#				else:
#					fix_image_arr.append([key_2,key])
if len(fix_image_arr)>0:
	for y in range(0, size[1]):
		for x in range(0, size[0]):
			pix = px[x,y]
			for i in fix_image_arr:
				if (pix == i[1]):
					px[x,y] = i[0]
	im.save("out.png")
	print "file saved to out.png"
	
print "No modifications necessary"
map_list = []
for key in mapping:
	map_list.append((mapping[key], key))
print map_list
node = create_tree(map_list)
huff = {}

# Recursively walk the tree down to the leaves,
#   assigning a code value to each symbol
def walk_tree(node, prefix="", code={}):
	if node[1].left is not None:
		if isinstance(node[1].left[1], HuffmanNode):
			walk_tree(node[1].left, prefix+"0")
		else:
			huff[node[1].left]=prefix+"0"
	if node[1].right is not None:
		if isinstance(node[1].right[1], HuffmanNode):
			walk_tree(node[1].right, prefix+"1")
		else:
			huff[node[1].right] = prefix+"1"
print(node[0])

code = walk_tree(node)
#node[1].preorder()
print "Huffman coding:"
print huff
final_str = ""
for y in range(0, size[1]):
	for x in range(0, size[0]):
		pix = px[x,y]
		for key in huff:
			if pix == key[1]:
				final_str+=huff[key]
print "\nBinary string, huffman encoded"
print final_str
#print int(final_str, 2)
count = 0
temp = ""
int_list = []
print "\nArray the contains the compressed pattern data"
for i in range(0, len(final_str)):

	if count%32 == 0 and count!=0:
		int_list.append(hex(int(temp,2)))
		temp = final_str[i]
	else:
		temp = temp + final_str[i]
	#print count
	count+=1
int_list.append(hex(int(temp.ljust(32, '0'),2)))
for i in int_list:
	if (len(i)<10):
		#print 10-len(i)
		strin = '0x'+'0'*(10-len(i))+i[2:10]
		i = strin
	else:
		i = i[0:10]
	print i+",",
print "\n"


max_depth=0
for key in huff:
	depth = 0
	for i in huff[key]:
		depth+=1
	if max_depth<depth:
		max_depth=depth
huff_tree = [-1] * (2**(max_depth+1) -1)
for key in huff:
	huff_tree_pos =0
	for i in huff[key]:
		if i=="1":
			huff_tree_pos = 2*huff_tree_pos+2
		else:
			huff_tree_pos = 2*huff_tree_pos+1
	huff_tree[huff_tree_pos] = closest_color(key[1][0],key[1][1],key[1][2])
	#((key[1][1] | 0x80) << 16) | ((key[1][0] | 0x80) <<  8) | key[1][2] | 0x80
	if max_depth <depth:
		max_depth=depth
print "Array representing huffman tree with GRB colors"
print huff_tree
