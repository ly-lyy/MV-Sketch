//---------by 611------------//
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <iostream>
#include <math.h>
#include <ctime>
#include <unordered_map>
#include <algorithm>
#include <set>
#include <iomanip>
#include "BOBHash32.h"
#pragma pack(1)

using namespace std;

#define KEY_SIZE 8
#define d 2
#define Real_Item_Nums 27121713


struct BUCKET//定义桶结构体
{
    uint64_t Key;//定义流标识（这里是取了8个字节）
    uint32_t V_Sum_Count;
    int Count;
};



BOBHash32 *Hash[d];//定义d个哈希函数（全局变量）


unordered_map <uint64_t,int> Real_KV_Map;//真实的键值对信息（MAP类型存储）
unordered_map <uint64_t,int> Estimated_KV_MAP;//算法估计的键值对信息（MAP类型存储）
vector < pair <uint64_t,int> > Real_KV_Vector;//真实的键值对信息（Vector类型存储，已按计数值从大到小排序）
vector < pair <uint64_t,int> > Estimated_KV_Vector;//估计的键值对信息（Vector类型存储，已按计数值从大到小排序）
set <uint64_t> All_Key;//存所有的key，方便查询


bool Cmp(const pair<uint64_t,int> x,const pair<uint64_t,int> y)//用于Map转Vector后，按照键值对的值从大到小排序
{
    return x.second>y.second;
}

void Get_All_Key()//获取数据流中所有种Key
{
    uint64_t temp_key;
    FILE *fp=fopen("src+dst IP.dat","rb");
    while(fread(&temp_key,1,KEY_SIZE,fp)==KEY_SIZE)
    {
        All_Key.insert(temp_key);
    }
}
void Map_To_Vector(unordered_map <uint64_t,int> &KV_Map,vector <pair<uint64_t,int>> &KV_Vector,FILE *fp)//MAP转Vector（参数里的文件是为了可视化，可以去掉）
{

    for(auto it=KV_Map.begin();it!=KV_Map.end();it++)//从完成的MAP中转存到Vector中，并按计数值从大到小进行排序（因为MAP的排序是按照key排的，但是需要用value排，而直接用vector又不能去重，所以为了既能去重又能排序，只能先用Map存再用Vector排）
    {
        KV_Vector.push_back(make_pair(it->first,it->second));
    }
    sort(KV_Vector.begin(),KV_Vector.end(),Cmp);
    for(int k=0;k!=KV_Vector.size();k++)
    {

        fwrite(&KV_Vector[k].first,KEY_SIZE,1,fp);
        fwrite(&KV_Vector[k].second,4,1,fp);
        //cout<<hex<<KV_Vector[k].first<<"\n";
        //cout<<dec<<KV_Vector[k].second<<"\n";
    }

}
void Get_Real_KV()//从dat文件中统计信息，统计出所有的键值对信息（真正的）
{
    uint64_t temp_key;
    FILE *fp=fopen("src+dst IP.dat","rb");
    FILE *Real_Result=fopen("Real_Result.dat","wb");
    fread(&temp_key,1,KEY_SIZE,fp);
    Real_KV_Map.insert(make_pair(temp_key,1));
    while(fread(&temp_key,1,KEY_SIZE,fp)==KEY_SIZE)
    {
        auto it=Real_KV_Map.find(temp_key);
        if(it!=Real_KV_Map.end())
        {
            Real_KV_Map[temp_key]++;
        }
        else
        {
            Real_KV_Map.insert(make_pair(temp_key,1));
        }
    }
    Map_To_Vector(Real_KV_Map,Real_KV_Vector,Real_Result);
}

void Create_Sketch_Init(BUCKET *bucket,int w)//Sketch的建立和初始化（这里相当于把二维数组看成一维数组来处理）
{
    for(int i=0;i<d*w;i++)
    {
        bucket[i].Count=0;
        bucket[i].Key=0;
        bucket[i].V_Sum_Count=0;
    }
    for(int i=0;i<d;i++)
    {
        Hash[i]=new BOBHash32(i+1000);
    }
}

void Print_Sketch_Info(BUCKET *bucket,int w)//输出二维表信息（用于编程时的验证，可以去掉）
{
    int j=0;//用来方便二维视图的
    //cout<<"第"<<j+1<<"行的信息："；
    for(int i=0;i<d*w;i++)
    {
        cout<<"{"<<bucket[i].Count<<" ";
        //cout<<" (Key ID:"<<bucket[i].Key<<") ";
        cout<<bucket[i].V_Sum_Count<<"} ";
        j++;
        //cout<<j;
        if(j==w){cout<<"\n";j=0;}
    }
}

void Update_Sketch(BUCKET *bucket,int w)//Sketch的更新
{
    int index=0,pos=0;
    uint32_t Hash_Value=0;
    FILE *fp=fopen("src+dst IP.dat","rb");
    //FILE *TEMP=fopen("temp.dat","wb");
    uint64_t temp_key;
    while(fread(&temp_key,1,KEY_SIZE,fp)==KEY_SIZE)
    {
        for(int i=0;i<d;i++)//控制哈希函数的循环
        {
            //fread(&temp,1,KEY_SIZE,fp);
            //bucket[i].Key=temp;
            //fwrite(&bucket[i].Key,8,1,TEMP);
            Hash_Value=Hash[i]->run((const char*)&temp_key,KEY_SIZE);//根据BOBHash产生哈希值
            //cout<<"哈希值为："<<Hash_Value<<"\n";
            index=Hash_Value%w;//将哈希值投入二维表中，获取当前行的索引值
            //cout<<index<<"\n";
            pos=i*w+index;
            bucket[pos].V_Sum_Count++;
            if(bucket[pos].Key==0)
            {
                bucket[pos].Key=temp_key;
                bucket[pos].Count=1;
            }
            else if(bucket[pos].Key==temp_key)
            {
                bucket[pos].Count++;
            }
            else
            {
                bucket[pos].Count--;
                if(bucket[pos].Count<=0)
                {
                    bucket[pos].Key=temp_key;
                    bucket[pos].Count=0;
                }
            }
        }
    }
}

void Query_and_Statistics(BUCKET *bucket,int w)//Sketch的查询和统计，即按照所用算法统计出估计的键值对信息
{
    FILE *Estimated_Result=fopen("Estimated_Result.dat","wb");
    int temp_value=0, index=0,pos=0,estimated_value=0;
    uint64_t temp_key;
    uint32_t Hash_Value=0;
    for(auto it=All_Key.begin();it!=All_Key.end();it++)//挨个遍历存放所有键的集合
    {
        for(int i=0;i<d;i++)//查询到当前键的计数值
        {
            Hash_Value=Hash[i]->run((const char*)&(*it),KEY_SIZE);
            index=Hash_Value%w;
            pos=i*w+index;
            if(bucket[pos].Key==*it)
            {
                temp_value=(bucket[pos].V_Sum_Count+bucket[pos].Count)/2;
            }
            else
            {
                temp_value=(bucket[pos].V_Sum_Count-bucket[pos].Count)/2;
            }
            if(i==0)
            {
                estimated_value=temp_value;
            }
            else
            {
                estimated_value=min(estimated_value,temp_value);
            }

        }
        temp_key=*it;
        Estimated_KV_MAP.insert(make_pair(temp_key,estimated_value));//把获取到的键值插入到MAP中
    }
    Map_To_Vector(Estimated_KV_MAP,Estimated_KV_Vector,Estimated_Result);
}

void Parameter_Threshold_Version(int threshold,vector <pair<uint64_t,int> > Estimated_Vector,vector <pair<uint64_t,int> > Real_Vector,FILE *fp)//参数统计（阈值版）
{
    int Real_Sx,Estimated_Sx;
    float Real_Heavy_hitter_Nums=0,Estimated_Heavy_hitter_Nums=0,Sum_ARE=0,Sum_AAE=0;

    //计算精确率precision和ARE
    for(int i=0;i<Real_Vector.size();i++)
    {
        if(Real_Vector[i].second>=threshold)
        {
            Real_Heavy_hitter_Nums++;
            Real_Sx=Real_Vector[i].second;
            for(int j=0;j<Estimated_Vector.size();j++)
            {
                if(Estimated_Vector[j].first==Real_Vector[i].first)
                {
                    Estimated_Sx=Estimated_Vector[j].second;
                    if(Estimated_Vector[j].second>=threshold)
                    {
                         Estimated_Heavy_hitter_Nums++;
                    }

                }
            }
            Sum_ARE=Sum_ARE+float(float(abs(Real_Sx-Estimated_Sx))/float(Real_Sx));
            Sum_AAE=Sum_AAE+float(float(abs(Real_Sx-Estimated_Sx)));
        }
        else break;

    }
    cout<<"真正的长流数量为："<<Real_Heavy_hitter_Nums<<"\n";
    cout<<"该算法识别出的长流数量为："<<Estimated_Heavy_hitter_Nums<<"\n";
    float precision=Estimated_Heavy_hitter_Nums/Real_Heavy_hitter_Nums;
    cout<<"**************************************\n";
    cout<<"重流识别的精确度(Precision)："<<precision*100.0<<"%\n";
    float ARE=float(Sum_ARE/float(abs(Real_Heavy_hitter_Nums)));
    float AAE=float(Sum_AAE/float(abs(Real_Heavy_hitter_Nums)));
    float log10_ARE=log10(ARE);
    cout<<"平均相对误差(ARE)："<<ARE<<"\n";
    cout<<"平均绝对误差(AAE)："<<AAE<<"\n";
    cout<<"**************************************\n";
    fprintf(fp,"%f,%f,%f,%f,",precision,ARE,AAE,log10_ARE);



    //计算回召率recall(就是命中率）
    Estimated_Heavy_hitter_Nums=0;
    float Correct_Estimated_Heavy_hitter_Nums=0;
    for(int i=0;i<Estimated_Vector.size();i++)
    {
        if(Estimated_Vector[i].second>=threshold)
        {
            Estimated_Heavy_hitter_Nums++;

            for(int j=0;j<Real_Vector.size();j++)
            {
                if(Real_Vector[j].first==Estimated_Vector[i].first&&Real_Vector[j].second>=threshold)
                {
                    Correct_Estimated_Heavy_hitter_Nums++;
                }
            }
        }
        else break;
    }
    cout<<"该算法识别出的长流数量为："<<Estimated_Heavy_hitter_Nums<<"\n";
    cout<<"其中命中的长流数量为："<<Correct_Estimated_Heavy_hitter_Nums<<"\n";
    float recall=Correct_Estimated_Heavy_hitter_Nums/Estimated_Heavy_hitter_Nums;
    cout<<"**************************************\n";
    cout<<"重流识别的回召率(Recall)："<<recall*100.0<<"%\n";
    cout<<"**************************************\n";

    //计算F1得分
    float f1_score=float((2.0*precision*recall)/precision+recall);
    cout<<"F1 Score为："<<f1_score<<"\n";
    cout<<"**************************************\n";

    fprintf(fp,"%f,%f\n",recall,f1_score);

}


void Parameter_Top_K_Version(int K,vector <pair<uint64_t,int> > Estimated_Vector,vector <pair<uint64_t,int> > Real_Vector,FILE *fp)//参数统计（Top-K版）
{

    int Real_Sx,Estimated_Sx;
    float Estimated_Heavy_hitter_Nums=0,Sum_ARE=0,Sum_AAE=0;

    //计算精确率precision和ARE
    for(int i=0;i<K;i++)
    {
        Real_Sx=Real_Vector[i].second;
        for(int j=0;j<K;j++)
        {
            if(Estimated_Vector[j].first==Real_Vector[i].first)
            {
                Estimated_Sx=Estimated_Vector[j].second;
                Estimated_Heavy_hitter_Nums++;
            }
        }
        Sum_ARE=Sum_ARE+float(float(abs(Real_Sx-Estimated_Sx))/float(Real_Sx));
        Sum_AAE=Sum_AAE+float(float(abs(Real_Sx-Estimated_Sx)));
    }
    cout<<"当前设定的Top-k的K值为："<<K<<"\n";
    cout<<"该算法识别正确的数量为："<<Estimated_Heavy_hitter_Nums<<"\n";
    float precision=Estimated_Heavy_hitter_Nums/K;
    float ARE=float(Sum_ARE/float(abs(K)));
    float AAE=float(Sum_AAE/float(abs(K)));
    float log10_ARE=log10(ARE);
    cout<<"**************************************\n";
    cout<<"重流识别的精确度(Precision)："<<precision*100.0<<"%\n";
    cout<<"平均相对误差(ARE)："<<ARE<<"\n";
    cout<<"平均绝对误差(AAE)："<<AAE<<"\n";
    cout<<"**************************************\n";

    //fprintf(fp,"%.2f\n",precision);
    fprintf(fp,"%f,%.6f,%f,%f\n",precision,ARE,AAE,log10_ARE);

}

int main()
{
    int Threshold[6]={10000,20000,30000,40000,50000,60000};
    int Top_K[6]={200,300,400,500,600,700};
    int Memory[6]={30,50,75,100,125,150};

    cout<<"请输入你的需求：\n";
    cout<<"找Top-K:   不同K值（K值为200—700不等） -------------  按0\n";
    cout<<"找Top-K:   不同内存（内存为30KB—150KB不等） -------------  按1\n";
    cout<<"找超过阈值T的：   不同阈值（阈值为500—5000不等） -------------  按2\n";
    cout<<"找超过阈值T的：   不同内存（内存为30KB—150KB不等） -------------  按3\n";

    int choose;
    cin>>choose;

    if(choose==0)
    {
        cout<<"输入想设置的内存条件：___(KB)\n";
        int m;
        cin>>m;

        int memory=m*1024;
        int w=floor(memory/(sizeof(BUCKET)*d));
        BUCKET *bucket=new BUCKET[w*d];


        FILE *fp = fopen("MV Sktech Various_k.csv", "w+"); //创建csv文件用于存储数据
        fprintf(fp,"内存(KB),K值,桶数,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE)\n");

        for(int i=0;i<6;i++)
        {
            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//更新所用时间
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//更新的吞吐量，单位是Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",m,Top_K[i],w*d,Update_Times,throughput);
            Parameter_Top_K_Version(Top_K[i],Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"更新的吞吐量(Throught)："<<throughput<<"Mbps\n";
            cout<<"更新所用时间(Update Time)："<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"桶数："<<w*d<<"\n";
            Estimated_KV_MAP.clear();
            Real_KV_Map.clear();
            All_Key.clear();
            //delete(bucket);
            vector < pair <uint64_t,int32_t> >().swap(Estimated_KV_Vector);
            vector < pair <uint64_t,int32_t> >().swap(Real_KV_Vector);
            cout<<"\n"<<"\n";
        }
    }

    else if(choose==1)
    {
        cout<<"输入想设置的K值条件：___\n";
        int k;
        cin>>k;

        FILE *fp = fopen("MV Sktech Various_memory_k.csv", "w+"); //创建csv文件用于存储数据
        fprintf(fp,"K值,内存(KB),桶数,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE)\n");


        for(int i=0;i<6;i++)
        {
            int memory=Memory[i]*1024;
            int w=floor(memory/(sizeof(BUCKET)*d));

            BUCKET *bucket=new BUCKET[w*d];


            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//更新所用时间
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//更新的吞吐量，单位是Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",k,memory,w*d,Update_Times,throughput);
            Parameter_Top_K_Version(k,Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"更新的吞吐量(Throught)："<<throughput<<"Mbps\n";
            cout<<"更新所用时间(Update Time)："<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"桶数："<<w*d<<"\n";
            Estimated_KV_MAP.clear();
            Real_KV_Map.clear();
            All_Key.clear();
            delete(bucket);
            vector < pair <uint64_t,int32_t> >().swap(Estimated_KV_Vector);
            vector < pair <uint64_t,int32_t> >().swap(Real_KV_Vector);
            cout<<"\n"<<"\n";

        }
    }
    else if(choose==2)
    {
        cout<<"输入想设置的内存条件：___(KB)\n";
        int m;
        cin>>m;
        int memory=m*1024;
        int w=floor(memory/(sizeof(BUCKET)*d));
        BUCKET *bucket=new BUCKET[w*d];


        FILE *fp = fopen("MV Sktech Various_T.csv", "w+"); //创建csv文件用于存储数据
        fprintf(fp,"内存(KB),阈值T,桶数,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE),ReCall,F1 Score\n");

        for(int i=0;i<6;i++)
        {
            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//更新所用时间
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//更新的吞吐量，单位是Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",m,Threshold[i],w*d,Update_Times,throughput);

            Parameter_Threshold_Version(Threshold[i],Estimated_KV_Vector,Real_KV_Vector,fp);


            cout<<"**************************************\n";
            cout<<"更新的吞吐量(Throught)："<<throughput<<"Mbps\n";
            cout<<"更新所用时间(Update Time)："<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"桶数："<<w*d<<"\n";
            Estimated_KV_MAP.clear();
            Real_KV_Map.clear();
            All_Key.clear();
            //delete(bucket);
            vector < pair <uint64_t,int32_t> >().swap(Estimated_KV_Vector);
            vector < pair <uint64_t,int32_t> >().swap(Real_KV_Vector);
            cout<<"\n"<<"\n";
        }

    }
    else if(choose==3)
    {
        cout<<"输入想设置的T值条件：___\n";
        int t;
        cin>>t;

        FILE *fp = fopen("MV Sktech Various_memory_T.csv", "w+"); //创建csv文件用于存储数据
        fprintf(fp,"T值,内存(KB),桶数,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE),ReCall,F1 Score\n");


        for(int i=0;i<6;i++)
        {
            int memory=Memory[i]*1024;
            int w=floor(memory/(sizeof(BUCKET)*d));

            BUCKET *bucket=new BUCKET[w*d];


            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//更新所用时间
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//更新的吞吐量，单位是Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",t,Memory[i],w*d,Update_Times,throughput);
            Parameter_Threshold_Version(t,Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"更新的吞吐量(Throught)："<<throughput<<"Mbps\n";
            cout<<"更新所用时间(Update Time)："<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"桶数："<<w*d<<"\n";
            Estimated_KV_MAP.clear();
            Real_KV_Map.clear();
            All_Key.clear();
            delete(bucket);

            vector < pair <uint64_t,int32_t> >().swap(Estimated_KV_Vector);
            vector < pair <uint64_t,int32_t> >().swap(Real_KV_Vector);
            cout<<"\n"<<"\n";

        }
    }







}





