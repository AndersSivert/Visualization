//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "Assignment7.h"
//---------------------------------------------------------------------------
#include "Properties.h"
#include "GLGeometryViewer.h"
#include "GeoXOutput.h"
//---------------------------------------------------------------------------

#include <limits>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>

IMPLEMENT_GEOX_CLASS( Assignment7, 0)
{
    BEGIN_CLASS_INIT( Assignment7 );
	ADD_SEPARATOR("Vectorfield file name")
	ADD_STRING_PROP(fileName, 0)
	
	ADD_SEPARATOR("Texture parameters")
	ADD_BOOLEAN_PROP(greyScale,0)
	ADD_INT32_PROP(xPowerOfTwo,0)
	ADD_INT32_PROP(yPowerOfTwo,0)
	ADD_INT32_PROP(textureSeed,0)
	//ADD_BOOLEAN_PROP(ContrastEnhancement,0)
	//ADD_FLOAT32_PROP(DesiredMean,0)
	//ADD_FLOAT32_PROP(DesiredDeviation,0)
	

	ADD_SEPARATOR("Runge-Kutta parameters")
	ADD_INT32_PROP(RKSteps,0)
	ADD_FLOAT32_PROP(RKStepSize,0)
	ADD_FLOAT32_PROP(minimumMagnitude,0)
	ADD_FLOAT32_PROP(ZeroThreshold,0)
	ADD_SEPARATOR("Kernel parameters")
	ADD_INT32_PROP(kernelLength,0)

	
	ADD_NOARGS_METHOD(Assignment7::ReadFieldFromFile)
	ADD_NOARGS_METHOD(Assignment7::CriticalPoints)
	ADD_NOARGS_METHOD(Assignment7::GenerateTexture)
	//ADD_NOARGS_METHOD(Assignment7::ClassicLIC)
	ADD_NOARGS_METHOD(Assignment7::FastLIC)
	//ADD_NOARGS_METHOD(Assignment7::EnhanceContrast)

	
	

    
}

QWidget* Assignment7::createViewer()
{
    viewer = new GLGeometryViewer();
    return viewer;
}

Assignment7::Assignment7()
{
    viewer = NULL;

	fileName = "C:\\Users\\Martin\\Desktop\\GeoX\\Assignment07\\Data\\Vector\\ANoise2CT4.am";		//Martin
	//fileName = "";	//Anders
	//fileName = "";	//Jim

	readField = false;
	
	xPowerOfTwo = 0;
	yPowerOfTwo = 0;
	
	textureSeed = 0;

	EulerSteps = 100;
	EulerStepSize = 0.1;
	
	RKSteps = 100;
	RKStepSize = 0.1;

	maxLength = 10;
	directionField = true;
	magnitudeColor = false;
	showPoints = true;
	grid = true;
	minimumMagnitude = 0.05;
	adjustArea = 0;
	adjustSteps = 5;
	maxAdjustment = 0.05;

	kernelLength = 10;

	ContrastEnhancement = false;
	DesiredMean = 0.5;
	DesiredDeviation = 0.1;

	ZeroThreshold = 0.00001;
	ClassifyCriticals = true;

}

Assignment7::~Assignment7() {}

/// Returns the next power of two
///	Kudos to ExampleExperimentFields
///	Unchanged --> consider importing instead?
namespace
{
    ///Returns the next power-of-two
    int32 NextPOT(int32 n)
    {
        n--;
        n |= n >> 1;   // Divide by 2^k for consecutive doublings of k up to 32,
        n |= n >> 2;   // and then or the results.
        n |= n >> 4;
        n |= n >> 8;
        n |= n >> 16;
        n++;           // The result is a number of 1 bits equal to the number
                       // of bits in the original number, plus 1. That's the
                       // next highest power of 2.
        return n;
    }
}

Vector2f Assignment7::RungeKuttaIntegration(Vector2f pos) {
	Vector2f p1 =  vField.sample(pos[0],pos[1]);
	Vector2f p2 =  vField.sample(pos[0]+p1[0]*RKStepSize/2,pos[1]+p1[1]*RKStepSize/2);
	Vector2f p3 =  vField.sample(pos[0]+p2[0]*RKStepSize/2,pos[1]+p2[1]*RKStepSize/2);
	Vector2f p4 =  vField.sample(pos[0]+p3[0]*RKStepSize,pos[1]+p3[1]*RKStepSize);
	return pos + (p1 + p2*2 + p3*2 + p4)*RKStepSize/6;
}
Vector2f Assignment7::RungeKuttaIntegration(Vector2f pos, bool forwards) {
	float dt;
	if(forwards) {
		dt = RKStepSize;
	} else {
		dt = -RKStepSize;
	}

	if(directionField) {
		Vector2f p1 =  vField.sample(pos[0],pos[1]);
		p1.normalize();
		Vector2f p2 =  vField.sample(pos[0]+p1[0]*dt/2,pos[1]+p1[1]*dt/2);
		p2.normalize();
		Vector2f p3 =  vField.sample(pos[0]+p2[0]*dt/2,pos[1]+p2[1]*dt/2);
		p3.normalize();
		Vector2f p4 =  vField.sample(pos[0]+p3[0]*dt,pos[1]+p3[1]*dt);
		p4.normalize();
		return pos + (p1 + p2*2 + p3*2 + p4)*dt/6;
	} else {
		Vector2f p1 =  vField.sample(pos[0],pos[1]);
		Vector2f p2 =  vField.sample(pos[0]+p1[0]*dt/2,pos[1]+p1[1]*dt/2);
		Vector2f p3 =  vField.sample(pos[0]+p2[0]*dt/2,pos[1]+p2[1]*dt/2);
		Vector2f p4 =  vField.sample(pos[0]+p3[0]*dt,pos[1]+p3[1]*dt);
		return pos + (p1 + p2*2 + p3*2 + p4)*dt/6;
	}
}

void Assignment7::ReadFieldFromFile(){
	if (!vField.load(fileName))
    {
		if(!sField.load(fileName)) {
			output << "Error loading field file " << fileName << "\n";
			return;
		} else {
			// If we have a scalar field, use the gradient as a vector field.
			vField.clear();
			vField.init(sField.boundMin(),sField.boundMax(),sField.dims());
			for(int x=0;x<sField.dims()[0];x++) {
				for(int y=0;y<sField.dims()[1];y++) {
					vField.setNode(x,y,sField.sampleGradient(sField.nodePosition(x,y)));
				}
			}
			readField = true;
		}
    } else {
		readField = true;
	}

}

void Assignment7::CriticalPoints() {
	//viewer->clear();
	AllCriticals.clear();
	if(!readField) {
		ReadFieldFromFile();
	}
	FindCriticalPoints();
	if(ClassifyCriticals) {
		ClassifyCriticalPoints();
		for(int i = 0; i < Source.size();i++) {
			Point2D point(Source[i][0],Source[i][1]);
			point.color = makeVector4f(1,0,0,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();
		}
		for(int i = 0; i < FocusRepelling.size();i++) {
			Point2D point(FocusRepelling[i][0],FocusRepelling[i][1]);
			point.color = makeVector4f(1,0.5,0,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();

		}
		for(int i = 0; i < Saddle.size();i++) {
			Point2D point(Saddle[i][0],Saddle[i][1]);
			point.color = makeVector4f(1,1,0,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();

		}
		for(int i = 0; i < Center.size();i++) {
			Point2D point(Center[i][0],Center[i][1]);
			point.color = makeVector4f(0,1,0,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();
		}
		for(int i = 0; i < Sink.size();i++) {
			Point2D point(Sink[i][0],Sink[i][1]);
			point.color = makeVector4f(0,0,1,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();

		}
		for(int i = 0; i < FocusAttracting.size();i++) {
			Point2D point(FocusAttracting[i][0],FocusAttracting[i][1]);
			point.color = makeVector4f(1,0,1,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();
		}
		ComputeSeparatrices();
		viewer->refresh();
	} else {
		for(int i = 0; i<AllCriticals.size(); i++) {
			
			Point2D point(AllCriticals[i][0],AllCriticals[i][1]);
			//point.position = AllCriticals[i];
			point.color = makeVector4f(1,0,0,1);
			//point.size = 20;
			viewer->addPoint(point);
			viewer->refresh();
		}
	}
}

bool Assignment7::IsZeroPossible(int x, int y) {
	bool xDiff = false;
	bool yDiff = false;
	for(int i = x; i<x+2; i++) {
		for(int j = y; j<y+2; j++) {
			if(vField.node(x,y)[0]*vField.node(i,j)[0]<=0) {
				xDiff = true;
			}
			if(vField.node(x,y)[1]*vField.node(i,j)[1]<=0) {
				yDiff = true;
			}
		}
	}
	return (xDiff&&yDiff);
}
bool Assignment7::IsZeroPossible(vector<Vector2f> points) {
	bool xDiff = false;
	bool yDiff = false;
	Vector2f startVector = vField.sample(points[0]);
	for(int i = 1; i<points.size();i++) {
		Vector2f fieldVector = vField.sample(points[i]);
		if(startVector[0]*fieldVector[0]<=0) {
			xDiff = true;
		}
		if(startVector[1]*fieldVector[1]<=0) {
			yDiff = true;
		}
		
	}
	return (xDiff&&yDiff);
}
void Assignment7::FindZero(vector<Vector2f> points) {
	Vector2f middle = points[0];
	for(int i = 1; i<points.size();i++) {
		middle += points[i];
	}
	middle = middle/points.size();

	if(vField.sample(middle).getSqrNorm()<ZeroThreshold) {
		//Found a zero! put it with the others
		foundCriticals.push_back(middle);
	} else {
		for(int i = 0; i<points.size();i++) {
			vector<Vector2f> newPoints;
			newPoints.push_back(middle);
			newPoints.push_back(points[i]);
			newPoints.push_back(makeVector2f(middle[0],points[i][1]));
			newPoints.push_back(makeVector2f(points[i][0],middle[1]));
			if(IsZeroPossible(newPoints)) {
				FindZero(newPoints);
			}
		}
	}
}

void Assignment7::FindCriticalPoints() {
	//For every cell...
	for(int x = 0; x<vField.dims()[0]-1;x++) {
		for(int y = 0; y<vField.dims()[1]-1;y++) {
			//Check if the cell possibly could contain a zero
			if(IsZeroPossible(x,y)) {
				foundCriticals.clear();
				//If so, find it
				vector<Vector2f> points;
				points.push_back(vField.nodePosition(x,y));
				points.push_back(vField.nodePosition(x+1,y));
				points.push_back(vField.nodePosition(x,y+1));
				points.push_back(vField.nodePosition(x+1,y+1));
				FindZero(points);
				if(foundCriticals.size()>0) {
					Vector2f pos;
					pos.setZero();
					for(int i = 0; i<foundCriticals.size();i++) {
						pos += foundCriticals[i];
					}
					pos = pos/foundCriticals.size();
					AllCriticals.push_back(pos);
				}
			}
		}
	}
}

void Assignment7::ClassifyCriticalPoints() {
	Source.clear();
	FocusRepelling.clear();
	Saddle.clear();
	Center.clear();
	Sink.clear();
	FocusAttracting.clear();

	for(int n = 0; n < AllCriticals.size(); n++) {
		Matrix2f Jacobian = vField.sampleJacobian(AllCriticals[n]);
		Vector2f Real;
		Vector2f Imaginary;
		Matrix2f vectors;
		Jacobian.solveEigenProblem(Real,Imaginary,vectors);
		if((Imaginary[0]==0)&&(Imaginary[1]==0)) { //Attracting, repelling and saddle

			if(Real[0]*Real[1]<0) {
				//Saddle, different sign on the real values
				Saddle.push_back(AllCriticals[n]);
			} else if(Real[0]<0) {
				//Source, both less than zero
				Source.push_back(AllCriticals[n]);
			} else if(Real[0]>0) {
				//Sink, both greater than zero
				Sink.push_back(AllCriticals[n]);
			} //If one or both are zero, then the theory doesn't tell us what it is
		} else {
			if(Real[0]>0) {
				//Repelling focus, real values greater than zero
				FocusRepelling.push_back(AllCriticals[n]);
			} else if(Real[0]<0) {
				//Attracting focus, real values greater than zero
				FocusAttracting.push_back(AllCriticals[n]);
			} else {
				//Center, real values zero
				Center.push_back(AllCriticals[n]);
			}
		}
	}
}

void Assignment7::ComputeSeparatrices() {
	Vector2f Real;
	Vector2f Imaginary;
	Matrix2f vectors;
	float maxLength = (vField.boundMax()-vField.boundMin()).getSqrNorm()*2;
	for(int n = 0; n<Saddle.size();n++) {
		vField.sampleJacobian(Saddle[n]).solveEigenProblem(Real,Imaginary,vectors);
		for(int i = 0; i<vectors.getRowsDim();i++) {
			for(int j = -1; j<2;j+=2) {
				float length = 0;
				
				int steps = 0;
				Vector2f Point = Saddle[n] + vectors[i] * RKStepSize * j;
				Vector2f oldPoint = vField.sample(Point);
				oldPoint.normalize();
				bool forwards = (oldPoint* vectors[i] * j) > 0;
				oldPoint = Saddle[n];
				while(vField.insideBounds(Point) && (length < maxLength) && (steps < RKSteps)) {

					viewer->addLine(oldPoint,Point,makeVector4f(0.5,1,1,1));
					length += (Point-oldPoint).getSqrNorm();

					oldPoint = Point;
					Point = RungeKuttaIntegration(Point,forwards);
					
					if(vField.sample(oldPoint).getSqrNorm()<ZeroThreshold
						&& vField.sample(Point).getSqrNorm()<ZeroThreshold) {
							length = maxLength;
					}
					steps++;
				}
			}
		}
	}
}

//Legacy methods below this point
//Could be useful for bonus points

void Assignment7::GenerateTexture() {
	viewer->clear();
	texture.clear();
	visited.clear();
	int iWidth = NextPOT(vField.boundMax()[0]-vField.boundMin()[0]);
	if(xPowerOfTwo>0) {
		iWidth = pow(2.0,xPowerOfTwo);
	}
	int iHeight = NextPOT(vField.boundMax()[1]-vField.boundMin()[1]);
	if(yPowerOfTwo>0) {
		iHeight = pow(2.0,yPowerOfTwo);
	}

	srand(textureSeed);
	
	texture.init(vField.boundMin(),vField.boundMax(),makeVector2ui(iWidth,iHeight));
	LICtexture.init(vField.boundMin(),vField.boundMax(),makeVector2ui(iWidth,iHeight));
	visited.init(vField.boundMin(),vField.boundMax(),makeVector2ui(iWidth,iHeight));
	if(greyScale) {
		for(size_t i = 0; i < iWidth;i++) {
			for(size_t j = 0; j < iHeight; j++) {
				visited.setNodeScalar(i,j,0.0);
				texture.setNodeScalar(i,j,(float)rand()/RAND_MAX);
			}
		}
	} else {
		for(size_t i = 0; i < iWidth;i++) {
			for(size_t j = 0; j < iHeight; j++) {
				visited.setNodeScalar(i,j,0.0);
				float value = (float)rand()/RAND_MAX;
				texture.setNodeScalar(i,j, (value > 0.5) ? 1 : 0);
			}
		}
	}
	RKStepSize = min((texture.boundMax()[0] - texture.boundMin()[0])/iWidth,(texture.boundMax()[1] - texture.boundMin()[1])/iHeight);
	viewer->setTextureGray(texture.getData());
	viewer->refresh();
}

void Assignment7::GenerateKernel() {
	kernelValues.clear();
	//TODO: Implement different kernels, kernel sizes etc.
	//Currently box kernel of size 5
	for(int i=0;i<kernelLength;i++) {
		kernelValues.push_back(1.0/kernelLength);
	}
}

vector<Vector2f> Assignment7::GenerateStreamLines(Vector2f startPoint, int pointNumber) {
	vector<Vector2f> line;
	line.push_back(startPoint);
	bool done = false;
	std::vector<Vector2f>::iterator it;
	int steps = 0;
	
	//Forwards interpolation:
	while(!done && steps < pointNumber) {
		it = line.end();
		Vector2f pos = line.back();

		Vector2f newPos = RungeKuttaIntegration(pos,true);
		//Discard conditions: out of bounds, too-low or zero magnitude
		if(vField.insideBounds(newPos)) {
			line.insert(it,newPos);
			if(vField.sample(newPos).getSqrNorm()<minimumMagnitude) {
				done = true;
			}
		} else {
			done = true;
		}
		steps++;
	}
	//int stepsRemaining = RKSteps - steps;
	done = false;
	steps = 0;

	while(!done && steps < pointNumber) {
		it = line.begin();
		Vector2f pos = line.front();

		Vector2f newPos = RungeKuttaIntegration(pos,false);
		//Discard conditions: out of bounds, too-low or zero magnitude
		if(vField.insideBounds(newPos)) {
			line.insert(it,newPos);
			if(vField.sample(newPos).getSqrNorm()<minimumMagnitude) {
				done = true;
			}
		} else {
			done = true;
		}
		steps++;
	}

	return line;
}

float Assignment7::convolveKernel(int startIndex, vector<Vector2f> line) {
	//This function assumes you've used the equidistant streamline method
	int kernelBoundaries = (int)kernelLength/2;
	float result = 0;
	for(int i = 0; i<kernelLength; i++) {
		//Calculate the index for the line sample
		int lineIndex = startIndex + i - kernelBoundaries;
		//Correct for out-of-bounds
		lineIndex = lineIndex < 0 ? 0 : lineIndex;
		lineIndex = lineIndex > line.size()-1 ? line.size()-1 : lineIndex;

		//Figure out what pixel said point belongs to:
		Vector2ui pixelIndex = texture.closestNode(line[lineIndex]);
		
		//Perform the convolution and add to the result
		result += kernelValues[i]*texture.nodeScalar(pixelIndex[0],pixelIndex[1]);
	}
	return result;
}

void Assignment7::ClassicLIC() {
	
	viewer->clear();
	vector<vector<Vector2f>> foundLines;
	vector<vector<Vector2f>> foundSampleLines;
	vector<Vector2f> line;

	LICtexture = texture;

	float segmentLength = min((texture.nodePosition(0,0)-texture.nodePosition(1,0)).getSqrNorm(),(texture.nodePosition(0,0)-texture.nodePosition(0,1)).getSqrNorm());
	GenerateKernel();
	int kernelHalf = (int)kernelLength/2;

	for(int x = 0; x < texture.dims()[0]; x++) {
		for(int y = 0; y < texture.dims()[1]; y++) {
			Vector2f startPoint = texture.nodePosition(x,y);
			
			line = GenerateStreamLines(startPoint, kernelLength);
			foundLines.push_back(line);
			int startIndex = find(line.begin(),line.end(),startPoint) - line.begin();
			LICtexture.setNodeScalar(x,y,convolveKernel(startIndex,line));
			
			//viewer->setTextureGray(LICtexture.getData());
			//viewer->refresh();
		}
	}
	/*for(int i = 0; i<foundLines.size(); i++) {
		for(int j = 0; j<foundLines[i].size()-1;j++) {
				viewer->addLine(foundLines[i][j],foundLines[i][j+1], makeVector4f(0,0,1,1));
		}
	}*/
	if(ContrastEnhancement) {
		EnhanceContrast();
	} else {
		viewer->setTextureGray(LICtexture.getData());
		viewer->refresh();
	}
}

void Assignment7::FastLIC() {
	viewer->clear();
	vector<vector<Vector2f>> foundLines;
	vector<vector<Vector2f>> foundSampleLines;
	vector<Vector2f> line;

	LICtexture = texture;
	TEMPtexture= texture;
	float segmentLength = min((texture.nodePosition(0,0)-texture.nodePosition(1,0)).getSqrNorm(),(texture.nodePosition(0,0)-texture.nodePosition(0,1)).getSqrNorm());
	GenerateKernel();
	//int iterator = 0;

	for(int x = 0; x < texture.dims()[0]; x++) {
		for(int y = 0; y < texture.dims()[1]; y++) {
			//Do not calculate streamlines for visited pixels
			if(visited.nodeScalar(x,y)>0) {
				continue;
			}
			//calculate the streamline
			line = GenerateStreamLines(texture.nodePosition(x,y), RKSteps);
			foundLines.push_back(line);
		
			//move kernel along line
			for(int n = 0; n < line.size(); n++) {
				//Find out what pixel we're visiting
				Vector2ui pixel = texture.closestNode(line[n]);
				//add one to the visit-counter for this pixel
				visited.setNodeScalar(pixel[0],pixel[1],visited.nodeScalar(pixel[0],pixel[1])+1);
				//convolve the kernel for this part of the line, and add it to the total of that pixel
				LICtexture.setNodeScalar(pixel[0],pixel[1], 
					LICtexture.nodeScalar(pixel[0],pixel[1]) + convolveKernel(n,line));
				
				//TEMPtexture.setNodeScalar(pixel[0],pixel[1],LICtexture.nodeScalar(pixel[0],pixel[1])/visited.nodeScalar(pixel[0],pixel[1]));
					
				
			} 
		}
	}
	//Adjust the values
	float value;
	for(int x = 0; x < texture.dims()[0]; x++) {
		for(int y = 0; y < texture.dims()[1]; y++) {
			value = (LICtexture.nodeScalar(x,y)-texture.nodeScalar(x,y))/visited.nodeScalar(x,y);
			LICtexture.setNodeScalar(x,y,value);
			visited.setNodeScalar(x,y,0.0);
		}
	}
	if(ContrastEnhancement) {
		EnhanceContrast();
	} else {
		viewer->setTextureGray(LICtexture.getData());
		viewer->refresh();
	}
}

void Assignment7::EnhanceContrast() {
	int n = 0;
	float P = 0;
	float mu = 0;
	for(int x = 0; x < LICtexture.dims()[0];x++) {
		for(int y = 0; y < LICtexture.dims()[1];y++) {
			float p = LICtexture.nodeScalar(x,y);
			if(p>0){
				n++;
				P += p*p;
				mu += p;
			}
		}
	}
	if(n<=1) {
		return;
	}
	mu = mu/n;
	float sd = sqrt((P-n*mu*mu)/(n-1));

	float stretching = DesiredDeviation/sd;// < 1) ? DesiredDeviation/sd : 1;

	for(int x = 0; x < LICtexture.dims()[0];x++) {
		for(int y = 0; y < LICtexture.dims()[1];y++) {
			float p = LICtexture.nodeScalar(x,y);
			LICtexture.setNodeScalar(x,y,DesiredMean + stretching*(p-mu));
		}
	}

	viewer->setTextureGray(LICtexture.getData());
	viewer->refresh();

}