#include "colang.h"

std::vector<VARLIST> varlist;

//ѹ���µ�������
//	name	����������
void scope::push(std::string zone)
{
	VARLIST vlist;
	vlist.zone = zone;
	varlist.push_back(vlist);
}

//����������
void scope::pop()
{
	varlist.pop_back();
}

//���ñ���
void scope::set(VARINFO vi)
{
	varlist[varlist.size() - 1].info[vi.name] = vi;
}

//��ȡ����
VARINFO scope::get(TOKEN token)
{
	for (int vi = varlist.size() - 1; vi >= 0; vi--)
	{
		if (varlist[vi].info.find(token.Value) == varlist[vi].info.end())
			continue; //δ�ҵ�ָ����������
		VARINFO vinfo = varlist[vi].info[token.Value];
		if (vinfo.value)
			return vinfo;
		//�����ǰ��function���壬����һ��ת��ȫ�ֱ��� 20240411 shanmin
		//	��ʱΪʲôһ���͵�ȫ�ֱ������������ԭ���� 20240827 shanmin
		if (varlist[vi].zone == "function")
			vi = 0;
	}
	//����
	std::vector<TOKEN> tmp;
	tmp.push_back(token);
	ErrorExit("ERROR: ����������", tmp);
}

//	THE END