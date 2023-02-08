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


struct BUCKET//����Ͱ�ṹ��
{
    uint64_t Key;//��������ʶ��������ȡ��8���ֽڣ�
    uint32_t V_Sum_Count;
    int Count;//ע�⣬��ƪMV-Sketch���漰��˥������ô����˥���������Ŀ��ܣ������Count��Ƴ��޷��������Ļ���һ��˥����������Count�ͻ�����쳣��������ƪ��Count�������޷�������������
};



BOBHash32 *Hash[d];//����d����ϣ������ȫ�ֱ�����


unordered_map <uint64_t,int> Real_KV_Map;//��ʵ�ļ�ֵ����Ϣ��MAP���ʹ洢��
unordered_map <uint64_t,int> Estimated_KV_MAP;//�㷨���Ƶļ�ֵ����Ϣ��MAP���ʹ洢��
vector < pair <uint64_t,int> > Real_KV_Vector;//��ʵ�ļ�ֵ����Ϣ��Vector���ʹ洢���Ѱ�����ֵ�Ӵ�С����
vector < pair <uint64_t,int> > Estimated_KV_Vector;//���Ƶļ�ֵ����Ϣ��Vector���ʹ洢���Ѱ�����ֵ�Ӵ�С����
set <uint64_t> All_Key;//�����е�key�������ѯ


bool Cmp(const pair<uint64_t,int> x,const pair<uint64_t,int> y)//����MapתVector�󣬰��ռ�ֵ�Ե�ֵ�Ӵ�С����
{
    return x.second>y.second;
}

void Get_All_Key()//��ȡ��������������Key
{
    uint64_t temp_key;
    FILE *fp=fopen("src+dst IP.dat","rb");
    while(fread(&temp_key,1,KEY_SIZE,fp)==KEY_SIZE)
    {
        All_Key.insert(temp_key);
    }
}
void Map_To_Vector(unordered_map <uint64_t,int> &KV_Map,vector <pair<uint64_t,int>> &KV_Vector,FILE *fp)//MAPתVector����������ļ���Ϊ�˿��ӻ�������ȥ����
{

    for(auto it=KV_Map.begin();it!=KV_Map.end();it++)//����ɵ�MAP��ת�浽Vector�У���������ֵ�Ӵ�С����������ΪMAP�������ǰ���key�ŵģ�������Ҫ��value�ţ���ֱ����vector�ֲ���ȥ�أ�����Ϊ�˼���ȥ����������ֻ������Map������Vector�ţ�
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
void Get_Real_KV()//��dat�ļ���ͳ����Ϣ��ͳ�Ƴ����еļ�ֵ����Ϣ�������ģ�
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

void Create_Sketch_Init(BUCKET *bucket,int w)//Sketch�Ľ����ͳ�ʼ���������൱�ڰѶ�ά���鿴��һά����������
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

void Print_Sketch_Info(BUCKET *bucket,int w)//�����ά����Ϣ�����ڱ��ʱ����֤������ȥ����
{
    int j=0;//���������ά��ͼ��
    //cout<<"��"<<j+1<<"�е���Ϣ��"��
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

void Update_Sketch(BUCKET *bucket,int w)//Sketch�ĸ���
{
    int index=0,pos=0;
    uint32_t Hash_Value=0;
    FILE *fp=fopen("src+dst IP.dat","rb");
    //FILE *TEMP=fopen("temp.dat","wb");
    uint64_t temp_key;
    while(fread(&temp_key,1,KEY_SIZE,fp)==KEY_SIZE)
    {
        for(int i=0;i<d;i++)//���ƹ�ϣ������ѭ��
        {
            //fread(&temp,1,KEY_SIZE,fp);
            //bucket[i].Key=temp;
            //fwrite(&bucket[i].Key,8,1,TEMP);
            Hash_Value=Hash[i]->run((const char*)&temp_key,KEY_SIZE);//����BOBHash������ϣֵ
            //cout<<"��ϣֵΪ��"<<Hash_Value<<"\n";
            index=Hash_Value%w;//����ϣֵͶ���ά���У���ȡ��ǰ�е�����ֵ
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

void Query_and_Statistics(BUCKET *bucket,int w)//Sketch�Ĳ�ѯ��ͳ�ƣ������������㷨ͳ�Ƴ����Ƶļ�ֵ����Ϣ
{
    FILE *Estimated_Result=fopen("Estimated_Result.dat","wb");
    int temp_value=0, index=0,pos=0,estimated_value=0;
    uint64_t temp_key;
    uint32_t Hash_Value=0;
    for(auto it=All_Key.begin();it!=All_Key.end();it++)//��������������м��ļ���
    {
        for(int i=0;i<d;i++)//��ѯ����ǰ���ļ���ֵ
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
        Estimated_KV_MAP.insert(make_pair(temp_key,estimated_value));//�ѻ�ȡ���ļ�ֵ���뵽MAP��
    }
    Map_To_Vector(Estimated_KV_MAP,Estimated_KV_Vector,Estimated_Result);
}

void Parameter_Threshold_Version(int threshold,vector <pair<uint64_t,int> > Estimated_Vector,vector <pair<uint64_t,int> > Real_Vector,FILE *fp)//����ͳ�ƣ���ֵ�棩
{
    int Real_Sx,Estimated_Sx;
    float Real_Heavy_hitter_Nums=0,Estimated_Heavy_hitter_Nums=0,Sum_ARE=0,Sum_AAE=0;

    //���㾫ȷ��precision��ARE
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
    cout<<"�����ĳ�������Ϊ��"<<Real_Heavy_hitter_Nums<<"\n";
    cout<<"���㷨ʶ����ĳ�������Ϊ��"<<Estimated_Heavy_hitter_Nums<<"\n";
    float precision=Estimated_Heavy_hitter_Nums/Real_Heavy_hitter_Nums;
    cout<<"**************************************\n";
    cout<<"����ʶ��ľ�ȷ��(Precision)��"<<precision*100.0<<"%\n";
    float ARE=float(Sum_ARE/float(abs(Real_Heavy_hitter_Nums)));
    float AAE=float(Sum_AAE/float(abs(Real_Heavy_hitter_Nums)));
    float log10_ARE=log10(ARE);
    cout<<"ƽ��������(ARE)��"<<ARE<<"\n";
    cout<<"ƽ���������(AAE)��"<<AAE<<"\n";
    cout<<"**************************************\n";
    fprintf(fp,"%f,%f,%f,%f,",precision,ARE,AAE,log10_ARE);



    //���������recall(���������ʣ�
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
    cout<<"���㷨ʶ����ĳ�������Ϊ��"<<Estimated_Heavy_hitter_Nums<<"\n";
    cout<<"�������еĳ�������Ϊ��"<<Correct_Estimated_Heavy_hitter_Nums<<"\n";
    float recall=Correct_Estimated_Heavy_hitter_Nums/Estimated_Heavy_hitter_Nums;
    cout<<"**************************************\n";
    cout<<"����ʶ��Ļ�����(Recall)��"<<recall*100.0<<"%\n";
    cout<<"**************************************\n";

    //����F1�÷�
    float f1_score=float((2.0*precision*recall)/precision+recall);
    cout<<"F1 ScoreΪ��"<<f1_score<<"\n";
    cout<<"**************************************\n";

    fprintf(fp,"%f,%f\n",recall,f1_score);

}


void Parameter_Top_K_Version(int K,vector <pair<uint64_t,int> > Estimated_Vector,vector <pair<uint64_t,int> > Real_Vector,FILE *fp)//����ͳ�ƣ�Top-K�棩
{

    int Real_Sx,Estimated_Sx;
    float Estimated_Heavy_hitter_Nums=0,Sum_ARE=0,Sum_AAE=0;

    //���㾫ȷ��precision��ARE
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
    cout<<"��ǰ�趨��Top-k��KֵΪ��"<<K<<"\n";
    cout<<"���㷨ʶ����ȷ������Ϊ��"<<Estimated_Heavy_hitter_Nums<<"\n";
    float precision=Estimated_Heavy_hitter_Nums/K;
    float ARE=float(Sum_ARE/float(abs(K)));
    float AAE=float(Sum_AAE/float(abs(K)));
    float log10_ARE=log10(ARE);
    cout<<"**************************************\n";
    cout<<"����ʶ��ľ�ȷ��(Precision)��"<<precision*100.0<<"%\n";
    cout<<"ƽ��������(ARE)��"<<ARE<<"\n";
    cout<<"ƽ���������(AAE)��"<<AAE<<"\n";
    cout<<"**************************************\n";

    //fprintf(fp,"%.2f\n",precision);
    fprintf(fp,"%f,%.6f,%f,%f\n",precision,ARE,AAE,log10_ARE);

}

int main()
{
    int Threshold[6]={10000,20000,30000,40000,50000,60000};
    int Top_K[6]={200,300,400,500,600,700};
    int Memory[6]={30,50,75,100,125,150};

    cout<<"�������������\n";
    cout<<"��Top-K:   ��ͬKֵ��KֵΪ200��700���ȣ� -------------  ��0\n";
    cout<<"��Top-K:   ��ͬ�ڴ棨�ڴ�Ϊ30KB��150KB���ȣ� -------------  ��1\n";
    cout<<"�ҳ�����ֵT�ģ�   ��ͬ��ֵ����ֵΪ500��5000���ȣ� -------------  ��2\n";
    cout<<"�ҳ�����ֵT�ģ�   ��ͬ�ڴ棨�ڴ�Ϊ30KB��150KB���ȣ� -------------  ��3\n";

    int choose;
    cin>>choose;

    if(choose==0)
    {
        cout<<"���������õ��ڴ�������___(KB)\n";
        int m;
        cin>>m;

        int memory=m*1024;
        int w=floor(memory/(sizeof(BUCKET)*d));
        BUCKET *bucket=new BUCKET[w*d];


        FILE *fp = fopen("MV Sktech Various_k.csv", "w+"); //����csv�ļ����ڴ洢����
        fprintf(fp,"�ڴ�(KB),Kֵ,Ͱ��,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE)\n");

        for(int i=0;i<6;i++)
        {
            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//��������ʱ��
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//���µ�����������λ��Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",m,Top_K[i],w*d,Update_Times,throughput);
            Parameter_Top_K_Version(Top_K[i],Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"���µ�������(Throught)��"<<throughput<<"Mbps\n";
            cout<<"��������ʱ��(Update Time)��"<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"Ͱ����"<<w*d<<"\n";
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
        cout<<"���������õ�Kֵ������___\n";
        int k;
        cin>>k;

        FILE *fp = fopen("MV Sktech Various_memory_k.csv", "w+"); //����csv�ļ����ڴ洢����
        fprintf(fp,"Kֵ,�ڴ�(KB),Ͱ��,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE)\n");


        for(int i=0;i<6;i++)
        {
            int memory=Memory[i]*1024;
            int w=floor(memory/(sizeof(BUCKET)*d));

            BUCKET *bucket=new BUCKET[w*d];


            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//��������ʱ��
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//���µ�����������λ��Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",k,memory,w*d,Update_Times,throughput);
            Parameter_Top_K_Version(k,Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"���µ�������(Throught)��"<<throughput<<"Mbps\n";
            cout<<"��������ʱ��(Update Time)��"<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"Ͱ����"<<w*d<<"\n";
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
        cout<<"���������õ��ڴ�������___(KB)\n";
        int m;
        cin>>m;
        int memory=m*1024;
        int w=floor(memory/(sizeof(BUCKET)*d));
        BUCKET *bucket=new BUCKET[w*d];


        FILE *fp = fopen("MV Sktech Various_T.csv", "w+"); //����csv�ļ����ڴ洢����
        fprintf(fp,"�ڴ�(KB),��ֵT,Ͱ��,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE),ReCall,F1 Score\n");

        for(int i=0;i<6;i++)
        {
            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//��������ʱ��
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//���µ�����������λ��Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",m,Threshold[i],w*d,Update_Times,throughput);

            Parameter_Threshold_Version(Threshold[i],Estimated_KV_Vector,Real_KV_Vector,fp);


            cout<<"**************************************\n";
            cout<<"���µ�������(Throught)��"<<throughput<<"Mbps\n";
            cout<<"��������ʱ��(Update Time)��"<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"Ͱ����"<<w*d<<"\n";
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
        cout<<"���������õ�Tֵ������___\n";
        int t;
        cin>>t;

        FILE *fp = fopen("MV Sktech Various_memory_T.csv", "w+"); //����csv�ļ����ڴ洢����
        fprintf(fp,"Tֵ,�ڴ�(KB),Ͱ��,Update Time,Throught(Mbps),Precision,ARE,AAE,Log10(ARE),ReCall,F1 Score\n");


        for(int i=0;i<6;i++)
        {
            int memory=Memory[i]*1024;
            int w=floor(memory/(sizeof(BUCKET)*d));

            BUCKET *bucket=new BUCKET[w*d];


            Create_Sketch_Init(bucket,w);
            clock_t time1=clock();
            Update_Sketch(bucket,w);
            clock_t time2=clock();

            double Update_Times=(double)(time2-time1)/CLOCKS_PER_SEC;//��������ʱ��
            double throughput=(Real_Item_Nums/1000000.0)/Update_Times;//���µ�����������λ��Mbps

            //Print_Sketch_Info();
            Get_All_Key();
            Query_and_Statistics(bucket,w);
            Get_Real_KV();

            fprintf(fp,"%d,%d,%d,%lf,%.5lf,",t,Memory[i],w*d,Update_Times,throughput);
            Parameter_Threshold_Version(t,Estimated_KV_Vector,Real_KV_Vector,fp);

            cout<<"**************************************\n";
            cout<<"���µ�������(Throught)��"<<throughput<<"Mbps\n";
            cout<<"��������ʱ��(Update Time)��"<<Update_Times<<"s\n";
            cout<<"**************************************\n";
            cout<<"Ͱ����"<<w*d<<"\n";
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





