1.�����ļ����ɷ�����
./hiergraphloader -n BJ_washed.cnode -e BJ_washed.cedge  -t 4 -l 8 -h BJ_washed_roadtree_t4_l8.idx  
t��l��ROADԭ�ĵĲ������ֱ��ʾ��֧�Ͳ���
2.���Է�����
hiernn_gtree -h BJ_roadtree_t4_l8.idx -x testFile

testFile�ļ���ʽ��
��һ�У�           num_obj        //  object ��Ŀ
������num_obj�У�  Obje_id        //��ʾ����vertex�ĵ��id    
query��Ŀ��        num_query      //query����Ŀ
������num_query��  query_id       //��ʾ����vertex�ĵ��id