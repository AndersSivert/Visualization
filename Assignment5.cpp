//---------------------------------------------------------------------------
#include "stdafx.h"
//---------------------------------------------------------------------------
#include "Assignment5.h"
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

IMPLEMENT_GEOX_CLASS( Assignment5, 0)
{
    BEGIN_CLASS_INIT( Assignment5 );
	ADD_SEPARATOR("Vectorfield file name")
	ADD_STRING_PROP(fileName, 0)
	
	ADD_SEPARATOR("Starting Line(s)")
	
    ADD_STRING_PROP(startingPoints, 0)
	ADD_BOOLEAN_PROP(randomPoints,0)
	ADD_BOOLEAN_PROP(grid,0)
	
	

    ADD_SEPARATOR("Euler parameters")
	ADD_INT32_PROP(EulerSteps,0)
	ADD_FLOAT32_PROP(EulerStepSize,0)

	ADD_SEPARATOR("Runge-Kutta parameters")
	ADD_INT32_PROP(RKSteps,0)
	ADD_FLOAT32_PROP(RKStepSize,0)

	
	ADD_SEPARATOR("Stream Line options")
	ADD_BOOLEAN_PROP(showPoints,0)
	ADD_BOOLEAN_PROP(magnitudeColor,0)
	ADD_BOOLEAN_PROP(directionField,0)
	ADD_FLOAT32_PROP(maxLength,0)
	ADD_FLOAT32_PROP(minimumMagnitude,0)
	
	ADD_INT32_PROP(adjustArea,0)
	ADD_INT32_PROP(adjustSteps,0)
	ADD_FLOAT32_PROP(maxAdjustment,0);

	ADD_NOARGS_METHOD(Assignment5::Comparison)
	ADD_NOARGS_METHOD(Assignment5::ReadFieldFromFile)
	ADD_NOARGS_METHOD(Assignment5::DrawStreamLines)

	
	

    
}

QWidget* Assignment5::createViewer()
{
    viewer = new GLGeometryViewer();
    return viewer;
}

Assignment5::Assignment5()
{
    viewer = NULL;
    RungeKutta = false;
	fileName = "C:\\Users\\Martin\\Desktop\\GeoX\\Assignment05\\Data\\ANoise2CT4.am";
	randomPoints=false;
	startingPoints="1,0";
	readField = false;

	EulerSteps = 100;
	EulerStepSize = 0.1;
	
	RKSteps = 100;
	RKStepSize = 0.1;

	maxLength = 10;
	directionField = false;
	magnitudeColor = false;
	showPoints = true;
	grid = true;
	minimumMagnitude = 0.05;
	adjustArea = 0;
	adjustSteps = 5;
	maxAdjustment = 0.05;

	colors.clear();
	for(int i=0;i<5;i++) {
		float c = (float)i/5;
		float s = 0.3;
		float c1 = exp(- c*c/(2*s*s))/(s*sqrt(2*3.1415));
		float c2 = exp(- (c-0.5)*(c-0.5)/(2*s*s))/(s*sqrt(2*3.1415));
		float c3 = exp(- (c-1)*(c-1)/(2*s*s))/(s*sqrt(2*3.1415));
		colors.push_back(makeVector4f(c1,c2,c3,1));
	}
	colorIndex = 0;
}

Assignment5::~Assignment5() {}


void Assignment5::Comparison() {

	viewer->clear();
	Vector2f pos;

	if(randomPoints)
	{
		float v = (float)rand() / RAND_MAX;
		v = v*2*3.1415;
		pos = makeVector2f(0.5*sin(v),cos(v));
	} else {
		int n = 0;
		string temp;
		stringstream ss(startingPoints);
		getline(ss,temp,',');
		pos[0] = stof(temp);
		getline(ss,temp,',');
		pos[1] = stof(temp);
	}



	Vector2f posRK = pos;
	for(int i = 0; i<RKSteps;i++)
	{
		Vector2f p1 =  Assignment5::GetFieldValue1(posRK);
		Vector2f p2 =  Assignment5::GetFieldValue1(posRK+p1*RKStepSize/2);
		Vector2f p3 =  Assignment5::GetFieldValue1(posRK+p2*RKStepSize/2);
		Vector2f p4 =  Assignment5::GetFieldValue1(posRK+p3*RKStepSize);
		Vector2f NewPosRK = posRK + (p1 + p2*2 + p3*2 + p4)*RKStepSize/6;
		viewer->addLine(posRK,NewPosRK,colors[0]);
		posRK = NewPosRK;
	}
	
	Vector2f posE = pos;
	for(int i = 0; i<EulerSteps;i++)
	{
		Vector2f NewPosE = posE + Assignment5::GetFieldValue1(posE)*EulerStepSize;
		viewer->addLine(posE,NewPosE,colors[colors.size()-1]);
		posE = NewPosE;
	}
	colorIndex = (colorIndex+3)%colors.size();
	viewer->refresh();
}

Vector2f Assignment5::GetFieldValue1(Vector2f pos)
{
	return makeVector2f(-pos[1],pos[0]/2);
}

Vector2f Assignment5::EulerIntegration(Vector2f pos) {
	return pos + field.sample(pos[0],pos[1])*EulerStepSize;
}

Vector2f Assignment5::EulerIntegration(Vector2f pos, bool forwards) {
	float dt;
	if(forwards) {
		dt = EulerStepSize;
	} else {
		dt = -EulerStepSize;
	}
	if(directionField){
		Vector2f sampled =field.sample(pos[0],pos[1]);
		sampled.normalize();
		return pos + sampled*dt;
	} else {
		return pos + field.sample(pos[0],pos[1])*dt;
	}
}

Vector2f Assignment5::RungeKuttaIntegration(Vector2f pos) {
	Vector2f p1 =  field.sample(pos[0],pos[1]);
	Vector2f p2 =  field.sample(pos[0]+p1[0]*RKStepSize/2,pos[1]+p1[1]*RKStepSize/2);
	Vector2f p3 =  field.sample(pos[0]+p2[0]*RKStepSize/2,pos[1]+p2[1]*RKStepSize/2);
	Vector2f p4 =  field.sample(pos[0]+p3[0]*RKStepSize,pos[1]+p3[1]*RKStepSize);
	return pos + (p1 + p2*2 + p3*2 + p4)*RKStepSize/6;
}
Vector2f Assignment5::RungeKuttaIntegration(Vector2f pos, bool forwards) {
	float dt;
	if(forwards) {
		dt = RKStepSize;
	} else {
		dt = -RKStepSize;
	}

	if(directionField) {
		Vector2f p1 =  field.sample(pos[0],pos[1]);
		p1.normalize();
		Vector2f p2 =  field.sample(pos[0]+p1[0]*dt/2,pos[1]+p1[1]*dt/2);
		p2.normalize();
		Vector2f p3 =  field.sample(pos[0]+p2[0]*dt/2,pos[1]+p2[1]*dt/2);
		p3.normalize();
		Vector2f p4 =  field.sample(pos[0]+p3[0]*dt,pos[1]+p3[1]*dt);
		p4.normalize();
		return pos + (p1 + p2*2 + p3*2 + p4)*dt/6;
	} else {
		Vector2f p1 =  field.sample(pos[0],pos[1]);
		Vector2f p2 =  field.sample(pos[0]+p1[0]*dt/2,pos[1]+p1[1]*dt/2);
		Vector2f p3 =  field.sample(pos[0]+p2[0]*dt/2,pos[1]+p2[1]*dt/2);
		Vector2f p4 =  field.sample(pos[0]+p3[0]*dt,pos[1]+p3[1]*dt);
		return pos + (p1 + p2*2 + p3*2 + p4)*dt/6;
	}
}

void Assignment5::ReadFieldFromFile(){
	if (!field.load(fileName))
    {
        output << "Error loading field file " << fileName << "\n";
        return;
    } else {
		readField = true;
	}

}

void Assignment5::DrawStreamLines()
{
	viewer->clear();
	if(!readField){
		ReadFieldFromFile();
	}
	
	float xMin = field.boundMin()[0];
	float yMin = field.boundMin()[1];
	float xMax = field.boundMax()[0];
	float yMax = field.boundMax()[1];
	
	//Step 1: determine seed points
	vector<Vector2f> startPoints;
	if(randomPoints) {
		int N = stoi(startingPoints);
		for(int i=0;i<N;i++) {
			Vector2f newPos;
			newPos[0] = (float)rand()*(xMax-xMin)/RAND_MAX + xMin;
			newPos[1] = (float)rand()*(yMax-yMin)/RAND_MAX + yMin;
			startPoints.push_back(newPos);
		}
	} else if(grid) {
		string temp;
		stringstream ss(startingPoints);
		vector<float> gridDimensions;
		while(getline(ss,temp,',')){
			gridDimensions.push_back(stof(temp));
		}
		int xDim;
		int yDim;
		if(gridDimensions.size()==1) { 
			xDim = gridDimensions[0]; 
			yDim = gridDimensions[0];
		} else {
			xDim = gridDimensions[0]; 
			yDim = gridDimensions[1];
		}
		if(xDim==1&&yDim==1) {
			Vector2f newPos;
			newPos[0] = 0.5*(xMax+xMin);
			newPos[1] = 0.5*(yMax+yMin);
			startPoints.push_back(newPos);
		} else {
			for(int x=1; x<=xDim;x++) {
				for(int y=1; y<=yDim;y++) {
					Vector2f newPos;
					newPos[0] = x*(xMax-xMin)/(xDim+1) + xMin;
					newPos[1] = y*(yMax-yMin)/(yDim+1) + yMin;
					startPoints.push_back(newPos);
				}
			}
		}
	} else {
		Vector2f newPos;
		string temp;
		stringstream ss(startingPoints);
		if(getline(ss,temp,',')) {
			newPos[0] = stof(temp);
		} else {
			newPos[0] = 0.5*(xMin+xMax);
		}
		if(getline(ss,temp,',')) {
			newPos[1] = stof(temp);
		} else {
			newPos[1] = 0.5*(yMin+yMax);
		}
				
		startPoints.push_back(newPos);
	}
	
	if(adjustSteps>0 && maxAdjustment>0 && adjustArea > 0) {
		//float moveDistance = RKStepSize*max((xMax-xMin)/field.dims()[0],(yMax-yMin)/field.dims()[1]);
		//float moveDistance = 2*maxAdjustment/(adjustSteps*(adjustSteps+1));
		for(int n=0;n<adjustSteps;n++) {
			float stepLength = 2*maxAdjustment*(adjustSteps-n)/(adjustSteps*(adjustSteps+1));
			for(int i=0;i<startPoints.size();i++) {
				float ownMagn = field.sample(startPoints[i]).getSqrNorm();

				//lower-bounds node index:
				int xIndex = (int)(startPoints[i][0]-xMin)/(xMax-xMin);
				int xLower = (xIndex - adjustArea + 1 < 0) 
							? 0 : xIndex - adjustArea + 1;
				int xUpper = (xIndex + adjustArea > field.dims()[0]-1) 
							? field.dims()[0]-1 : xIndex + adjustArea;
				int yIndex = (int)(startPoints[i][1]-yMin)/(yMax-yMin);
				int yLower = (yIndex - adjustArea + 1 < 0) 
							? 0 : yIndex - adjustArea + 1;
				int yUpper = (yIndex + adjustArea > field.dims()[1]-1) 
							? field.dims()[1]-1 : yIndex + adjustArea;
				Vector2f movement;
				movement.setZero();
				
				//For all nodes in the cell
				for(int x = xLower; x <= xUpper; x++) {
					for(int y = yLower; y <= yUpper; y++) {
						//Find direction vector from position to node
						Vector2f temp = field.nodePosition(x,y)
										- startPoints[i];
						
						//temp[0] = x-0.5;
						//temp[1] = y-0.5;
						float dist = temp.getSqrNorm();
						dist = (dist == 0) ? 1 : dist;
						temp.normalize();
						
						//weigh it with the difference between the magnitude in the
						// point and the magnitude in the node. Move towards higher
						//magnitudes and away from lower.
						//output << "x: " << x << ", y " << y << "\n";
						movement += temp*(field.node(x,y).getSqrNorm()
										  - ownMagn)/dist;
					}
				}
				//output << "movement vector " << movement << "\n";
				//Now we have the weighted sum. Normalize this vector...
				movement.normalize();
				//And set its length to the above calculated one
				//movement = movement * moveDistance;//*(1.1*adjustSteps-n)/adjustSteps;
				movement = movement * stepLength;
				//Check if we're inside the bounds:
				if(field.insideBounds(startPoints[i]+movement)) {
					//If we are, move the point
					startPoints[i] = startPoints[i] + movement;
				}
			}
			
		}
	}
	
	vector<vector<Vector2f>> AllStreamLines;
	for(int i=0;i<startPoints.size();i++) {
		vector<Vector2f> StreamLine;
		StreamLine.push_back(startPoints[i]);
		bool dontSwap = false;
		bool forwards = true;
		int stepsTaken = 0;
		float length = 0;
		bool done = false;
		std::vector<Vector2f>::iterator it;

		while(!done && stepsTaken<RKSteps && length<maxLength) {
			Vector2f pos;
			if(forwards){
				pos = StreamLine.back();
				it = StreamLine.end();
			} else {
				pos = StreamLine.front();
				it = StreamLine.begin();
			}
			Vector2f newPos = RungeKuttaIntegration(pos,forwards);
			//Discard conditions: out of bounds, too-low or zero magnitude
			bool stopDirection = false;
			if(newPos[0]<xMin || newPos[0]>xMax || newPos[1]<yMin || newPos[1]>yMax) {
					stopDirection = true;
			} else {
				StreamLine.insert(it,newPos);
			
				//Too slow:
				if(field.sample(newPos).getSqrNorm() < minimumMagnitude) {
					stopDirection = true;
				}
				length += (newPos - pos).getSqrNorm();
				stepsTaken+=1;
			}
			if(dontSwap) {
				if(stopDirection) {
					done = true;
				} 
			} else {
				forwards = !forwards;
				if(stopDirection) {
					dontSwap = true;
				}
			} 

		}
		AllStreamLines.push_back(StreamLine);
	}
	if(magnitudeColor) {
		float maxMagnitude = 0;

		for(int i=0;i<field.dims()[0];i++) {
			for(int j=0;j<field.dims()[1];j++) {
				maxMagnitude = max(maxMagnitude,field.node(i,j).getSqrNorm());
			}
		}
		if (maxMagnitude == 0) {
			output << "all nodes are of magnitude 0" << "\n";
			maxMagnitude = 1;
		} 
		float c;
		float s = 0.3;
		float c1;
		float c2;
		float c3;
		for(int i=0;i<AllStreamLines.size();i++) {
			for(int n=1;n<AllStreamLines[i].size();n++) {
				c = 0.5*field.sample(AllStreamLines[i][n-1]).getSqrNorm();
				c += 0.5*field.sample(AllStreamLines[i][n]).getSqrNorm();
				c = c/maxMagnitude;
		
				c1 = exp(- c*c/(2*s*s))/(s*sqrt(2*3.1415));
				c2 = 0.9*exp(- (c-0.5)*(c-0.5)/(2*(s-0.05)*(s-0.05)))/(s*sqrt(2*3.1415));
				c3 = exp(- (c-1)*(c-1)/(2*(s-0.05)*(s-0.05)))/(s*sqrt(2*3.1415));
				viewer->addLine(AllStreamLines[i][n-1],AllStreamLines[i][n],makeVector4f(c3,c2,c1,1));
			}
		}
	} else {
		for(int i=0;i<AllStreamLines.size();i++) {
			for(int n=1;n<AllStreamLines[i].size();n++) {
				viewer->addLine(AllStreamLines[i][n-1],AllStreamLines[i][n],makeVector4f(1,1,1,1));
			}
		}
	}
	if(showPoints) {
		for(int i=0;i<AllStreamLines.size();i++) {
			for(int n=1;n<AllStreamLines[i].size();n++) {
				Point2D newPoint(startPoints[i]);
				newPoint.color = makeVector4f(0,1,0,1);
				viewer->addPoint(newPoint);
				viewer->addLine(AllStreamLines[i][n-1],AllStreamLines[i][n],makeVector4f(1,1,1,1));
			}
		}
		for(int i=0;i<startPoints.size();i++) {
			Point2D newPoint(startPoints[i]);
			newPoint.color = makeVector4f(1,0,0,1);
			viewer->addPoint(newPoint);
		}
		
	}
	viewer->refresh();
}