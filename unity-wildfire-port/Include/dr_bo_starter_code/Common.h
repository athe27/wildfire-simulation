#ifndef __Common_h__
#define __Common_h__
#include <vector>
#include <list>
#include <queue>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include "eigen/Eigen/Dense"
#include "eigen/Eigen/Sparse"
#include "glm/glm.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtx/quaternion.hpp"

using namespace glm;

//////////////////////////////////////////////////////////////////////////
////Eigen basic types

////Eigen vector type alias
template<class T,int d> using Vector=Eigen::Matrix<T,d,1>;
template<class T,int d> using Matrix=Eigen::Matrix<T,d,d>;

#define Declare_Eigen_Vector_Types(type,t)		\
using Vector1##t=Eigen::Matrix<type,1,1>;       \
using Vector2##t=Eigen::Vector2##t;             \
using Vector3##t=Eigen::Vector3##t;             \
using Vector4##t=Eigen::Vector4##t;             \
using VectorX##t=Eigen::VectorX##t;

#define Declare_Eigen_Matrix_Types(type,t)		\
using Matrix1##t=Eigen::Matrix<type,1,1>;       \
using Matrix2##t=Eigen::Matrix2##t;             \
using Matrix3##t=Eigen::Matrix3##t;             \
using Matrix4##t=Eigen::Matrix4##t;             \
using MatrixX##t=Eigen::MatrixX##t;  

Declare_Eigen_Vector_Types(int,i)
Declare_Eigen_Vector_Types(float,f)
Declare_Eigen_Vector_Types(double,d)
Declare_Eigen_Matrix_Types(int,i)
Declare_Eigen_Matrix_Types(float,f)
Declare_Eigen_Matrix_Types(double,d)

#define Declare_Eigen_Types(type,t)     \
using real=type;						\
using Vector1=Eigen::Matrix<real,1,1>;	\
using Vector2=Eigen::Vector2##t;        \
using Vector3=Eigen::Vector3##t;        \
using Vector4=Eigen::Vector4##t;        \
using VectorX=Eigen::VectorX##t;        \
using Matrix2=Eigen::Matrix2##t;        \
using Matrix3=Eigen::Matrix3##t;        \
using Matrix4=Eigen::Matrix4##t;        \
using MatrixX=Eigen::MatrixX##t;        \
using Quaternion=Eigen::Quaternion##t;  \
using AngleAxis=Eigen::AngleAxis##t;

#ifdef USE_FLOAT
Declare_Eigen_Types(float,f)
#else
Declare_Eigen_Types(double,d)
#endif

////Eigen vector alias macros
#define Typedef_VectorD(d)		\
using VectorD=Vector<real,d>
#define Typedef_VectorDi(d)		\
using VectorDi=Vector<int,d>
#define Typedef_VectorDii(d)	\
using VectorD=Vector<real,d>;	\
using VectorDi=Vector<int,d>;
#define Typedef_VectorEi(d)		\

//////////////////////////////////////////////////////////////////////////
////Container alias

template<class T> using Array=std::vector<T>;
template<class T> using List=std::list<T>;
template<class T,class CMP=std::less<T> > using Heap=std::priority_queue<T,std::vector<T>,CMP>;
template<class T_KEY,class T> using Hashtable=std::unordered_map<T_KEY,T>;
template<class T_KEY,class T> using HashtableMultiValue=std::unordered_multimap<T_KEY,T>;
template<class T_KEY> using Hashset=std::unordered_set<T_KEY>;

template<class T1,class T2> using Pair=std::pair<T1,T2>;
template<class T> using ArrayPtr=std::shared_ptr<Array<T> >;

using size_type=Array<int>::size_type;
using uchar=unsigned char;
using ushort=unsigned short;

////Array with fixed size
template<class T,int n> using ArrayF=std::array<T,n>;
constexpr int Pow(int x,int p){return p==1?x:x*Pow(x,p-1);}
constexpr int Factorial(int n){return n<=1?1:(n*Factorial(n-1));}
template<class T,int d> using ArrayF2P=ArrayF<T,Pow(2,d) >;
template<class T,int d> using ArrayF3P=ArrayF<T,Pow(3,d) >;

//////////////////////////////////////////////////////////////////////////
////Analytical geometry

template<int d> class Box
{using VectorD=Vector<real,d>;using VectorDi=Vector<int,d>;
public:
	VectorD min_corner,max_corner;
	Box(const VectorD& _min=VectorD::Zero(),const VectorD& _max=VectorD::Zero()):min_corner(_min),max_corner(_max){}
};

//////////////////////////////////////////////////////////////////////////
////Dimension conversion

template<class T,int d1,int d2> void Dim_Conversion(const Vector<T,d1>& input,/*result*/Vector<T,d2>& output,const T filled_value=(T)0)
{
	int n=d1<d2?d1:d2;
	for(int i=0;i<n;i++)output[i]=input[i];
	for(int i=n;i<d2;i++)output[i]=filled_value;
}

template<class T,int d1,int d2> void Dim_Conversion_Array(const Array<Vector<T,d1> >& input,/*result*/Array<Vector<T,d2> >& output,const T filled_value=(T)0)
{
	for(size_type i=0;i<input.size();i++){
		Dim_Conversion<T,d1,d2>(input[i],output[i],filled_value);}
}

#endif
