#ifndef __Particles_h__
#define __Particles_h__
#include <memory>
#include <map>
#include <set>
#include <fstream>
#include "Common.h"
#include "File.h"

#define Declare_Attribute(T,A,a,Rebind_A) \
	public:T& A(const int i){return (*a)[i];}\
	const T& A(const int i) const {return (*a)[i];}\
	Array<T>* A(){return a.get();}\
	const Array<T>* A() const {return a.get();}\
	ArrayPtr<T> A##Ptr(){return a;}\
	const ArrayPtr<T> A##Ptr() const {return a;}\
	Array<T>& A##Ref(){return *a;}\
	const Array<T>& A##Ref() const {return *a;}\
	void Rebind_A(Array<T>* _a){assert(_a->size()==Size());a.reset(_a);}\
    protected:ArrayPtr<T> a;public:

template<int d> class Attributes
{using VectorD=Vector<real,d>;
public:
	void Resize(const size_type& size)
	{
		if(size==0){
			for(auto& e:vector_attributes)e.second->clear();
			for(auto& e:scalar_attributes)e.second->clear();
			for(auto& e:integer_attributes)e.second->clear();}
		else{for(auto& e:vector_attributes)e.second->resize(size,VectorD::Zero());
			for(auto& e:scalar_attributes)e.second->resize(size,(real)0);
			for(auto& e:integer_attributes)e.second->resize(size,0);}
	}

	void Reserve(const size_type& size)
	{
		for(auto& e:vector_attributes)e.second->reserve(size);
		for(auto& e:scalar_attributes)e.second->reserve(size);
		for(auto& e:integer_attributes)e.second->reserve(size);
	}

	void Add_Element()
	{
		for(auto& e:vector_attributes)e.second->push_back(VectorD::Zero());
		for(auto& e:scalar_attributes)e.second->push_back((real)0);
		for(auto& e:integer_attributes)e.second->push_back(0);
	}

	void Add_Elements(const size_type n)
	{
		for(auto& e:vector_attributes)e.second->resize(e.second->size()+n,VectorD::Zero());
		for(auto& e:scalar_attributes)e.second->resize(e.second->size()+n,(real)0);
		for(auto& e:integer_attributes)e.second->resize(e.second->size()+n,0);
	}

	template<class T> using AttMapPair=Pair<std::string,ArrayPtr<T> >;

	bool Add(const std::string name,ArrayPtr<VectorD>& val)
	{vector_attributes.insert(AttMapPair<VectorD>(name,val));return true;}

	bool Add(const std::string name,ArrayPtr<real>& val)
	{scalar_attributes.insert(AttMapPair<real>(name,val));return true;}

	bool Add(const std::string name,ArrayPtr<int>& val)
	{integer_attributes.insert(AttMapPair<int>(name,val));return true;}

	void Clear()
	{
		vector_attributes.clear();
		scalar_attributes.clear();
		integer_attributes.clear();
	}

protected:
	std::map<std::string,ArrayPtr<VectorD> > vector_attributes;
	std::map<std::string,ArrayPtr<real> > scalar_attributes;
	std::map<std::string,ArrayPtr<int> > integer_attributes;
};

template<int d> class Particles
{using VectorD=Vector<real,d>;
public:
	Attributes<d> attributes;
	Declare_Attribute(VectorD,X,x,Rebind_X);	////position
    Declare_Attribute(VectorD,V,v,Rebind_V);	////velocity
    Declare_Attribute(VectorD,F,f,Rebind_F);	////force
    Declare_Attribute(real,M,m,Rebind_M);		////mass
    Declare_Attribute(real,C,c,Rebind_C);		////color
	Declare_Attribute(real,R,r,Rebind_R);		////radius
	Declare_Attribute(real,P,p,Rebind_P);		////pressure
	Declare_Attribute(real,D,den,Rebind_D);		////density

	Particles(){Add_Attributes();}

	virtual void Add_Attributes()
	{
		Add_Attribute("x",x);
		Add_Attribute("v",v);
		Add_Attribute("f",f);
		Add_Attribute("m",m);
		Add_Attribute("c",c);
		Add_Attribute("r",r);
		Add_Attribute("p",p);
		Add_Attribute("den",den);
	}

	virtual void Resize(const int size){attributes.Resize((size_type)size);}
	virtual void Reserve(const int size){attributes.Reserve((size_type)size);}
	virtual int Add_Element(){attributes.Add_Element();return Size()-1;}
	virtual int Add_Elements(int n){attributes.Add_Elements(n);return Size()-n;}
	virtual int Size() const {return (int)(*x).size();}

    ////IO
	void Write_Binary(std::ostream &output) const
	{
		int n=Size();File::Write_Binary(output,n);
		if(n>0){
			File::Write_Binary_Array(output,&(*X())[0],n);
			File::Write_Binary_Array(output,&(*V())[0],n);
			File::Write_Binary_Array(output,&(*F())[0],n);
			File::Write_Binary_Array(output,&(*M())[0],n);
			File::Write_Binary_Array(output,&(*C())[0],n);}
	}

	void Read_Binary(std::istream &input)
	{
		int n=0;File::Read_Binary(input,n);
		Resize(n);
		if(n>0){
			File::Read_Binary_Array(input,&(*X())[0],n);
			File::Read_Binary_Array(input,&(*V())[0],n);
			File::Read_Binary_Array(input,&(*F())[0],n);
			File::Read_Binary_Array(input,&(*M())[0],n);
			File::Read_Binary_Array(input,&(*C())[0],n);}
		Add_Attributes();
	}

	void Write_To_File_3d(const std::string& file_name) const
	{
		Particles<3> p3;p3.Resize(Size());
		Dim_Conversion_Array<real,d,3>(*X(),*p3.X(),(real)0);
		Dim_Conversion_Array<real,d,3>(*V(),*p3.V(),(real)0);
		Dim_Conversion_Array<real,d,3>(*F(),*p3.F(),(real)0);
		*p3.M()=*M();
		*p3.C()=*C();
		File::Write_Binary_To_File(file_name,p3);
	}

protected:
	template<class T> void Add_Attribute(const std::string name,ArrayPtr<T> & a)
	{
		if(a==nullptr)a.reset(new Array<T>());
		attributes.Add(name,a);
	}
};
#endif
