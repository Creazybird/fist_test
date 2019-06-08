<template>
  <div>
    <Header />
    <div class="app-container">
      <div class="buttons">
        <el-row>
          <el-button
            size="small"
            type="info"
            class="button-add"
            @click="toHome"
          >
            主页</el-button>
          <el-button @click='reset'>重置密码</el-button>
          <el-button
            size="small"
            class="button-add"
            @click="add"
          >
            增加成员</el-button>
          <el-upload ref="upload" action="/" :show-file-list="false" :on-change="importExcel" :auto-upload="false" style="display: inline;">
            <el-button slot="trigger" size="small"  >
              批量导入
            </el-button>
          </el-upload>
          <el-button :loading="downloadLoading" size="small" icon="document" @click="handleDownload">
            批量导出
          </el-button>
           <el-button :loading="downloadLoading" size="small" icon="document" @click="handleDownloadTemplate">
            下载模板
          </el-button>      

          <el-button :loading="downloadLoading" size="small" icon="document" @click="upload">
            上传测试
          </el-button>
          <!-- <el-button  @click="dialogVisible = true" size="small" icon="document" :loading="downloadLoading">选择批量导出</el-button> -->
            <el-dialog
              title="提示"
              :visible.sync="dialogVisible"
              width="30%"
              :before-close="handleClose">
              <span>这是一段信息</span>
              <template>
                  <el-checkbox :indeterminate="isIndeterminate" v-model="checkAll" @change="handleCheckAllChange">全选</el-checkbox>
                  <div style="margin: 15px 0;"></div>
                  <el-checkbox-group v-model="checkedAttributes" @change="handleCheckedCitiesChange">
                    <el-checkbox v-for="attribute in Attributes" :label="attribute" :key="attribute">{{attribute}}</el-checkbox>
                  </el-checkbox-group>
                </template>
              <span slot="footer" class="dialog-footer">
                <el-button @click="Cancel">取 消</el-button>
                <!-- <el-button type="primary" @click="dialogVisible = false">确 定</el-button> -->
                <el-button type="primary" @click="Download">确 定</el-button>
              </span>
            </el-dialog>
          <el-button :loading="downloadLoading" size="small" icon="document" @click="search" class="highsearch" type="primary">
            高级搜索
          </el-button>
           <div class="select">
        <el-select v-model="searchs" size="small" placeholder="请选择检索字段">
          <el-option
            v-for="item in options"
            :key="item.value"
            :label="item.label"
            :value="item.value"
          />
        </el-select>
        <el-input
          v-model="searchValue"
          size="small"
          placeholder="请输入内容"
          prefix-icon="el-icon-search"
          class="search"
        />
        <el-button
          size="small"
          type="primary"
          class="button-add"
          @click="screen"
        >
          检索</el-button>
      </div>
        </el-row>
        <el-row v-if="judgeRole()" class="fourbutton">
          <div class="searchOption1">
          <el-button size="small" @click="checkingExperts"  >
            审核专家
          </el-button>
          <el-button size="small" @click="deletecheckingExperts" >
            审核删除
          </el-button>
          <el-button size="small" @click="deletedExperts" >
            后备专家库
          </el-button>
          </div>
          <div class="fourbutton">
             
          </div>
        </el-row>
      </div>
      <!-- <div class="select"> -->
        <!-- <span style="color: #889aa4; font-size: 14px;">选择年龄段:</span>
        <el-input
          v-model="lowAge"
          size="small"
          style="width: 55px;"
        />
        <span>-</span>
        <el-input
          v-model="upAge"
          size="small"
          style="width: 55px; margin-right: 10px;"
        /> -->
        <!-- <el-select v-model="searchs" size="small" placeholder="请选择检索字段">
          <el-option
            v-for="item in options"
            :key="item.value"
            :label="item.label"
            :value="item.value"
          />
        </el-select>
        <el-input
          v-model="searchValue"
          size="small"
          placeholder="请输入内容"
          prefix-icon="el-icon-search"
          class="search"
        />
        <el-button
          size="small"
          type="primary"
          class="button-add"
          @click="screen"
        >
          检索</el-button>
      </div> -->
      <el-table
        ref="multipleTable"
        v-loading="listLoading"
        :data="list"
        stripe
        :header-cell-style="{background:'#F5F5F5'}"
        element-loading-text="拼命加载中"
        border
        fit
        highlight-current-row
        @selection-change="handleSelectionChange"
      >
        <el-table-column
          type="selection"
          align="center"
          width="40"

        />
        <el-table-column prop="name" align="center" label="姓名" width="95" />
        <el-table-column align="center" label="照片" width="135">
          <template slot-scope="scope">
            <img :src="scope.row.picture" width="100" height="100">
          </template>
        </el-table-column>
        <el-table-column label="性别" width="60" align="center">
          <template slot-scope="scope">
            {{ scope.row.gender }}
          </template>
        </el-table-column>
        <el-table-column label="出生年月" width="160" align="center">
          <template slot-scope="scope">
            {{ scope.row.birthday }}
          </template>
        </el-table-column>
        <el-table-column label="研究方向" width="120" align="center">
          <template slot-scope="scope">
            {{scope.row.researchDirection}}
          </template>
        </el-table-column>
          <el-table-column label="行政职务" width="120" align="center">
          <template slot-scope="scope">
            {{scope.row.administrativePost}}
          </template>
        </el-table-column>
        <el-table-column prop="address" align="center" label="通讯地址" />
        <el-table-column
          label="状态"
          width="100"
          align="center"
        >
        <template slot-scope="scope">
          <!-- <template v-if="scope.row.status === '删除审核中'" slot-scope="scope"> -->
          <template  v-if="scope.row.status === '删除审核中'">
            <!-- <el-popover trigger="hover" placement="top"> -->
              <!-- <p>删除理由: {{ scope.row.reason }}</p>
                  <el-button size="small" v-if="judgeRole()" @click="passDelete(scope.row)">审核通过</el-button>
                  <el-button size="small" v-if="judgeRole()" type="danger" @click="refuseDelete(scope.row)">审核驳回</el-button> -->
              <div slot="reference" class="name-wrapper">
                <el-tag size="medium" :type="showTheme(scope.row)" key='1'>{{ scope.row.status }}</el-tag>
              </div>
            <!-- </el-popover> -->
          </template>
         
          <template v-else-if="(scope.row.status !== '删除审核中') && judgeRole() && (scope.row.status === '已删除')" >
            <el-tag
              :type="showTheme(scope.row)"
              disable-transitions key='2'>后备专家</el-tag>
          </template> 
       
          <!-- <template v-if="(scope.row.status' !== '删除审核中') && (scope.row.status !== '已删除')" slot-scope="scope"> -->
            <template v-else >
            <el-tag
              :type="showTheme(scope.row)"
              disable-transitions key='3'>{{scope.row.status}}</el-tag>
            </template>
          </template>
  
           <!-- 刚进来时的页面 ，对于管理员和用户都还没进行过删除操作 此时所有条录的状态是空状态 -->
         
       
        </el-table-column>
        <el-table-column label="操作" width="250" align="center">
          <template slot-scope="scope">
            <el-button
              size="mini"
              @click="toedit(scope.row)"
              v-if="judgeEdit(scope.row)"
            >
              编辑</el-button>
              <!-- v-if="(scope.row.status !== '已删除')&&!((checkingButton)||(deletecheckingButton))" -->
                            <!-- v-if="(checkingButton)||(deletecheckingButton)" -->
              <!-- v-if="(scope.row.status !== '已删除')&&!((checkingButton)||(deletecheckingButton))" -->

            <el-button size="mini" type="danger" @click="handleDelete(scope.$index, scope.row)" v-if="judgeDelete(scope.row)">
              删除
            </el-button> 
            <el-button v-if="(checkingButton)||(deletecheckingButton)" size="mini" type="primary" @click="todetail(scope.row)">
              审核
            </el-button>
            <el-button v-else size="mini" type="primary" @click="todetail(scope.row)">
              查看详情
            </el-button>  
          </template>
        </el-table-column>
      </el-table>
      <el-pagination
        :page-size="20"
        :pager-count="11"
        :current-page.sync="page"
        layout="prev, pager, next"
        :total="totalPage*20"
        class="page"
        @current-change="handleCurrentChange(page)"
      />
    </div>
  </div>
</template>

<script>
import Header from '../../components/Header/index'
import Cookie from '../../utils/cookie'
import XLSX from 'xlsx'
import FileSaver from 'file-saver'
import { filter } from 'minimatch';
import infoVue from '../form/info.vue';

export default {
  components: {
    Header,
    FileSaver,
    XLSX
  },
  data() {
    return {
      checkAll: false,
      isIndeterminate: true,
      checkedAttributes:['姓名','性别','出生日期','工作单位','状态','学术研究方向','职称'],
      Attributes:['姓名','性别','出生日期','民族','办公电话','手机号码','传真号码','电子邮箱','担任导师','第一外语','第一外语水平','第二外语','第二外语水平','最后学历','最后学位','现任专业技术职务','通讯地址','邮政编码','学术兼职','个人简历','著作','论文','皮书成果','理论文章','主持项目','荣誉奖励','工作单位','状态','学术研究方向','职称'],
      dialogVisible:false,
      list: [],
      listLoading: true,
      totalPage: null,
      page: 1,
      searchs: '',
      statusMap : [" ","审核中", "审核通过", "审核未通过","删除审核中", "已删除"],
      statusThemeMap : ['','primary','success', 'danger', 'warning', 'danger'],
      options: [{
        value: 'Name',
        label: '姓名'
      }, {
        value: 'Gender',
        label: '性别'
      },
      {
        value: 'Address',
        label: '工作单位'
      },
      {
        value: 'Status',
        label: '状态'
      },
      {
        value: 'ResearchDirection',
        label: '学术研究方向'
      },
      {
        value: 'administrativePost',
        label: '职称'
      }],
      searchValue: '',
      lowAge: '',
      upAge: '',
      xlsxJson: null,
      multipleSelection: [],
      downloadLoading: false,
      deleteButton:false,
      checkingButton:false,
      deletecheckingButton:false,
      findAllInfo:[]
    }
  },
  created() {
    this.handleCurrentChange(1)
  },
  mounted() {
    this.handleCurrentChange(1)
  },
  methods: {
    upload(){
      this.$router.push({path:'/upload'})
    },
    //下载模板
    handleDownloadTemplate(){
      window.open(url)//文件路径名
    },
    
    Download(){
      this.dialogVisible=false
      if (this.multipleSelection.length) {
        this.handleDownload()
      }
      else{
         this.$message({
          message: '请至少选择一项需要导出的数据',
          type: 'warning'
        })
      }
    },
    Cancel(){
      this.dialogVisible=false
      this.checkedAttributes=['姓名','性别','出生日期','工作单位','状态','学术研究方向','职称']
    },
     handleCheckAllChange(val) {
        this.checkedAttributes = val ? this.Attributes : [];
        this.isIndeterminate = false;
      },
      handleCheckedCitiesChange(value) {
        let checkedCount = value.length;
        this.checkAll = checkedCount === this.Attributes.length;
        this.isIndeterminate = checkedCount > 0 && checkedCount < this.Attributes.length;
        console.log('选择的属性'+this.checkedAttributes)
      },
    handleClose(done) {
        this.$confirm('确认关闭？')
          .then(_ => {
            done();
          })  
          .catch(_ => {});
      },
    // reset(){this.$router.push({path:'/'})},
    reset(){ this.$router.push({ path: `/reset/9` })},
    toggleSelection(rows) {
      if (rows) {
        rows.forEach(row => {
          this.$refs.multipleTable.toggleRowSelection(row)
        })
      } else {
        this.$refs.multipleTable.clearSelection()
      }
    },
    judgeRole(){
      // console.log("thisadmin:"+Cookie.getCookie('role'))
      if(Cookie.getCookie('role') === 'Superadmin' || Cookie.getCookie('role') === 'Admin'){
        //&& this.form.status === 1
        return true;
      }
      return false;
    },
    judgeDelete(row){
      if(this.judgeRole()&&(row.status==='审核通过'||row.status==='删除审核中'||row.status==='审核未通过'))
      {
        return true;
      }
      else if(!this.judgeRole()&&(row.status==='审核通过'||row.status==='审核未通过')){
        return true;
      }
      else{
        return false;
      }
    },
    judgeEdit(row){
      if(this.judgeRole()&&row.status==='审核通过')
      {
        return true
      }
      else if(!this.judgeRole()&&(row.status==='审核未通过'))
      {
        return true
      }
      else{
        return false
      }

    },
    passDelete(row){
      const token=`Bearer ${Cookie.getCookie('lan_token')}`;
      fetch(`/api/Experts/approve/${row.expertId}`,{
        method:'DELETE',
        headers:{
          Authorization:token
        }
        }).then(res => {
          if(res.ok){
            return res.json()
          }
          }).then(res => {
            this.$message({
              message:'删除成功',
              type:'success'
            })
          this.handleCurrentChange(this.page)
          })
      },
    refuseDelete(row){
      const token=`Bearer ${Cookie.getCookie('lan_token')}`;
      fetch(`/api/Experts/disapprove/${row.id}`,{
        method:'DELETE',
        headers:{
          Authorization:token
        }
      }).then(res => {
        if(res.ok){
          return res.json()
        }
        }).then(res => {
          this.$message({
            message:'已取消删除',
            type:'success'
          })
          this.handleCurrentChange(this.page)
        })
    },
    showTheme(row){
      return this.statusThemeMap[this.statusMap.indexOf(row.status)]
    },
    handleSelectionChange(val) {
      this.multipleSelection = val
    },
    add() {
      this.$router.push({ path: `/new` })
    },
    screen() {
      this.page=1
      this.handleCurrentChange(this.page)
    },
    todetail(row) {
      this.$router.push({ path: `/manage/info/${row.expertId}` })
    },
    toedit(row) {
      this.$router.push({ path: `/manage/edit/${row.expertId}` })
    },
    handleDelete(index, row) {
      const token = `Bearer ${Cookie.getCookie('lan_token')}`
      const role = Cookie.getCookie('role')
      let deleteReason = ""
      if(role === 'Commonadmin'){//Commonadmin 代表普通管理员
          this.$prompt('请输入删除理由', '提示', {
          confirmButtonText: '确定',
          cancelButtonText: '取消',
          closeOnClickModal:false
        }).then(({ value }) => {
          deleteReason = value
          this.$message({
            type: 'success',
            message: '已向上一级发送删除请求'
          });
          fetch(`/api/Experts/${row.expertId}`, {
            method: 'DELETE',
            headers: {
              Authorization: token,
              'Content-Type': 'application/json'
            },
            body: JSON.stringify({Reason: deleteReason})//发送了自己要删除的原因
          }).then(res => {
            if (res.ok) {
              this.handleCurrentChange(this.page)
            }
          })
        }).catch(() => {
          this.$message({
            type: 'info',
            message: '取消输入'
          });       
        });
      }else{//else 这里就是超级管理员
        fetch(`/api/Experts/${row.expertId}`, {
          method: 'DELETE',
          headers: {
            Authorization: token,
          'Content-Type': 'application/json'
          },
          body: JSON.stringify({})
        }).then(res => {
          if (res.ok) {
            this.handleCurrentChange(this.page)
          }
        })
      }
    },
    importExcel(file){
       console.log(file)
       
       const types = file.name.split('.')[1]
       const fileType = ['xlsx', 'xlc', 'xlm', 'xls', 'xlt', 'xlw', 'csv'].some(item => item === types)
         if (!fileType) {
          alert('格式错误！请重新选择')
          return
      }
      let error=0
      let formData = new FormData();
      console.log(file)
      formData.append("file", file.raw);
      console.log(formData)
      const token = `Bearer ${Cookie.getCookie('lan_token')}`
      // console.log('令牌+'+token)
      fetch(`/api/Experts/batch`, {
        method: 'POST',
        headers: {
          Authorization: token,
          // "Content-Type": "multipart/form-data"
        },
        body: formData
      }).then(res => {
        if (res.ok);       
        else {
          error = 1
        }
      })
    if (error === 0) {
      this.$alert('导入成功', {
        confirmButtonText: '确定'
      }).then(() => {
        this.handleCurrentChange(1)
      })
    } else {
      this.$alert('导入失败，请重新尝试', {
        confirmButtonText: '确定'
      })
    }
      
    }
    // importExcel(file) {
    //   // let file = file.files[0] // 使用传统的input方法需要加上这一步
    //   const types = file.name.split('.')[1]
    //   const fileType = ['xlsx', 'xlc', 'xlm', 'xls', 'xlt', 'xlw', 'csv'].some(item => item === types)
    //   if (!fileType) {
    //     alert('格式错误！请重新选择')
    //     return
    //   }
    //   this.file2Xce(file).then(tabJson => {
    //     if (tabJson && tabJson.length > 0) {
    //       this.xlsxJson = tabJson[0].sheet
    //       // console.log('取出表头：',tabJson[0].sheet.header)
    //         let error = 0
    //         var translate=[]
    //       this.xlsxJson.map(sheet => {//遍历除了头行的每一行数据
    //             console.log('sheet==='+JSON.stringify(sheet))
    //              var keyMap={姓名:'name',性别:'gender',出生日期:'birthday',民族:'nation',办公电话:'telephone',手机号码:'mobile',传真号码:'fax',电子邮箱:'email',担任导师:'isSupervisor',职称:'administrativePost',
    //              第一外语:'firstForeignLang',第一外语水平:'firstForeignLangLevel',第二外语:'firstForeignLang',第二外语水平:'secondForeignLangLevel',最后学历:'highestAcademic',最后学位:'highestDegree',
    //              学术研究方向:'researchDirection',现任专业技术职务:'technicalPosition',工作单位:'organization',通讯地址:'address',邮政编码:'postcode',学术兼职:'academicJob',个人简历:'resume',
    //              著作:'monograph',论文:'thesis',皮书成果:'book',理论文章:'essay',主持项目:'project',荣誉奖励:'honor'}
    //             var obj=Object.keys(sheet).reduce((newData,key)=>{
    //             let newKey=keyMap[key]||key
    //             newData[newKey]=sheet[key]
    //             return newData
    //             },{})
    //             sheet=obj 
    //             sheet.isSupervisor=(sheet.isSupervisor==='是') ?true :false;
    //             sheet.gender =  (sheet.gender === '男' ) ? 1 : 2;
    //         console.log('sheet'+sheet)
    //         translate.push(sheet)
    //       })
          
    //       const token = `Bearer ${Cookie.getCookie('lan_token')}`
    //        console.log('上传的数据'+JSON.stringify(translate))
    //       console.log(this.xlsxJson)
    //         fetch(`/api/Experts/batch`, {
    //           method: 'POST',
    //           headers: {
    //             Authorization: token,
    //             'Content-Type': 'application/json'
    //           },
    //           body: JSON.stringify(translate)
    //         }).then(res => {
    //           if (res.ok)
    //           {
               
    //           }
    //           else {
    //             error = 1
    //           }
    //         })
    //       if (error === 0) {
    //         this.$alert('添加成功', {
    //           confirmButtonText: '确定'
    //         }).then(() => {
    //           this.handleCurrentChange(1)
    //         })
    //       } else {
    //         this.$alert('添加失败，请重新尝试', {
    //           confirmButtonText: '确定'
    //         })
    //       }
    //     }
    //   })
    // },
    ,
    file2Xce(file) {
      return new Promise(function(resolve, reject) {
        const reader = new FileReader()
        reader.onload = function(e) {
          const data = e.target.result
          this.wb = XLSX.read(data, {
            type: 'binary'
          })
          const result = []
          this.wb.SheetNames.forEach((sheetName) => {
            result.push({
              sheetName: sheetName,
              sheet: XLSX.utils.sheet_to_json(this.wb.Sheets[sheetName])
            })
          })
          resolve(result)
        }
        reader.readAsBinaryString(file.raw)
      })
    },
//  handleDownload_1(){
//    if(this.multipleSelection.length)
//    {
//      const tHeader=['姓名','性别','出生日期','民族','办公电话','手机号码','传真号码','电子邮箱','担任导师','职称','第一外语','第一外语水平','第二外语','第二外语水平','最后学历','最后学位',
//      '学术研究方向','现任专业技术职务','工作单位','通讯地址','邮政编码','学术兼职','个人简历','著作','论文','皮书成果','理论文章','主持项目','荣誉奖励']
//      const Engish_header=[name,gender,birthday,nation,telephone,mobile,fax,email,isSupervisor,administrativePost,firstForeignLang,firstForeignLangLevel,secondForeignLang,secondForeignLangLevel,highestAcademic,highestDegree,
//      researchDirection,technicalPosition,address,postcode,academicJob,resume,monograph,thesis,book,essay,project,honor]
//      const filterVal=Engish_header
//       this.getData().then(res=>{//直接导出所有单位机构的数据
//             const list=res
//             const data = this.formatJson(filterVal, list)
//             excel.export_json_to_excel({
//             header: tHeader,
//             data,
//             filename: this.filename
//           })
//           this.downloadLoading = false
//           // this.$refs.multipleTable.clearSelection()
//         })
//    }
//   else{
//         this.$message({
//           message: '请至少选择一项需要导出的数据',
//           type: 'warning'
//         })
//   }
//  },
// async getData_1(){
//       var alldata=[]
//           const token=`Bearer ${Cookie.getCookie('lan_token')}`;
//           await fetch(`/api/Experts/####`, {    fetch(`/api/Experts/Get`)
//             method:'GET',
//             headers:{
//               Authorization:token
//             }
//             }).then(res => {
//               if(res.ok){
//                 return res.json()
//               }
//               }).then(res => {
//                 alldata=res
//               })
   
//        alldata.map(data => {
//               console.log(data)
//               data.gender = (data.gender > 1 ? "女" : "男")
//               data.isSupervisor=(data.isSupervisor==true ? '是':'否')
//               data.status = this.statusMap[data.status]
//               // console.log("1:status----"+data.status)
//               data.birthday=this.tranlateDate(data.birthday)
//             })
//       return alldata
//     },
handleDownload(){
   const token=`Bearer ${Cookie.getCookie('lan_token')}`;
     fetch(`/api/Experts/batch`, {
            method:'GET',
            headers:{
              Authorization:token
            }
            }).then(res => {
              if(res.ok){
                return res.json()
                console.log(res)
              }
              })
}
//  handleDownload() {
//       if (this.multipleSelection.length) {
//         this.downloadLoading = true
//         import('@/vendor/Export2Excel').then(  excel => {
//           console.log('选择导出的属性'+this.checkedAttributes)
//           const data_1={姓名:'name',性别:'gender',学术研究方向:'researchDirection',职称:'administrativePost',工作单位:'address',状态:'status',
//           出生日期:'birthday',民族:'nation',办公电话:'telephone',手机号码:'mobile',传真号码:'fax',电子邮箱:'email',担任导师:'isSupervisor',第一外语:'firstForeignLang',
//           第一外语水平:'firstForeignLangLevel',第二外语:'secondForeignLang',第二外语水平:'secondForeignLangLevel',最后学历:'highestAcademic',最后学位:'highestDegree',
//           现任专业技术职务:'technicalPosition',通讯地址:'address',邮政编码:'postcode',学术兼职:'academicJob',个人简历:'resume',著作:'monograph',论文:'thesis',皮书成果:'book',
//           理论文章:'essay',主持项目:'project',荣誉奖励:'honor' }//去掉了birthday
//           let filter_data=[]
//           for(var i in this.checkedAttributes)
//           {
//             filter_data[i]=data_1[this.checkedAttributes[i]]
//           }
//           const tHeader=this.checkedAttributes
//           const filterVal=filter_data
//          var id_List=[]
//          for(var i in this.multipleSelection)
//           {
//               id_List.push(this.multipleSelection[i].id)
//           }
//           this.getData(id_List).then(res=>{``
//             const list=res
//             const data = this.formatJson(filterVal, list)
//             excel.export_json_to_excel({
//             header: tHeader,
//             data,
//             filename: this.filename
//           })
//           this.downloadLoading = false
//           this.$refs.multipleTable.clearSelection()
//         })
//           })
//       } else {
//         this.$message({
//           message: '请至少选择一项需要导出的数据,并且选择需要导出的字段,默认导出字段为。。。。',
//           type: 'warning'
//         })
//       }
//     },
,
    //获取数据 重要的一点就是让他返回一个Promise对象  这样我就可以.then().then().then()....
     async getData(id){
      var alldata=[]
      for(var i in id)
      {
          const token=`Bearer ${Cookie.getCookie('lan_token')}`;
          await fetch(`/api/Experts/${id[i]}`, {
            method:'GET',
            headers:{
              Authorization:token
            }
            }).then(res => {
              if(res.ok){
                return res.json()
              }
              }).then(res => {
                this.findAllInfo[i]=res
                alldata[i]=res
              })
      }
       alldata.map(data => {
              console.log(data)
              data.gender = (data.gender > 1 ? "女" : "男")
              // data.isSupervisor=(data.isSupervisor==true ? '是':'否')
              data.status = this.statusMap[data.status]
              // console.log("1:status----"+data.status)
              data.birthday=this.tranlateDate(data.birthday)
            })
      return alldata
    },


    formatJson(filterVal, jsonData) {
      return jsonData.map(v => filterVal.map(j => v[j]))
    },
    // handleCurrentChange(page) {
    //   this.listLoading = true
    //   const token = `Bearer ${Cookie.getCookie('lan_token')}`
    //   let str = `/api/Experts?Page=${page}&Limit=20`
    //   if (this.upAge !== '' && this.lowAge !== '') {
    //     str = str + `&Age=${this.lowAge}-${this.upAge}`
    //   }
    //   if (this.searchValue !== '' && this.searchs !== '') {
    //     let result
    //     if (this.searchs === 'Gender') {
    //       this.searchValue === '男' ? (result = 1) : (result = 2)
    //     } else if (this.searchs === 'Status') {
    //       const StatusMap = [' ', '审核中', '审核通过', '审核未通过']
    //       result = StatusMap.indexOf(this.searchValue)
    //       if (result === -1) {
    //         this.$message({
    //           message: '请填写审核中，审核通过和审核未通过'
    //         })
    //       }
    //     } else {
    //       result = this.searchValue
    //     }
    //     str = str + `&${this.searchs}=${result}`
    //   }
    //   fetch(str, {
    //     method: 'GET',
    //     headers: {
    //       Authorization: token,
    //       'Content-Type': 'application/json'
    //     }
    //     }).then(res => {
    //       if(res.ok){
    //         return res.json()
    //       }
    //       }).then(res => {
    //         this.list = res.experts,
    //         this.list.map(data => {
    //           data.gender = (data.gender > 1 ? "女" : "男")
    //           data.status = this.statusMap[data.status]
    //           data.birthday=this.tranlateDate(data.birthday)
    //         }),

    //         this.totalPage = res.totalPage,
    //         this.listLoading = false
    //     })
    // },
     handleCurrentChange(page) {
      console.log('page'+page)
      this.listLoading = true
      const token = `Bearer ${Cookie.getCookie('lan_token')}`
      // let str = `/api/Experts?Page=${page}&Limit=20&RangeStatus=1,4`
       let str = `/api/Experts?Draft=true`
      // if(this.deleteButton===true){
      //     str = str + `&Status=5`
      //     str= str+ this.mixSearch()
      //     console.log("str:"+str)
      // }
      // else if(this.checkingButton===true)
      // {
      //   str=str+`&Status=1`
      //   str= str+ this.mixSearch()
      //   console.log("str:"+str)
      // }
      // else if(this.deletecheckingButton===true)
      // {
      //   str=str+`&Status=4`
      //   str= str+ this.mixSearch()
      // }
      // else{
        //   if (this.upAge !== '' && this.lowAge !== '') {
        //   str = str + `&Age=${this.lowAge}-${this.upAge}`
        // }
        // if (this.searchValue !== '' && this.searchs !== '') {
        //   let result
        //   if (this.searchs === 'Gender') {
        //     this.searchValue === '男' ? (result = 1) : (result = 2)
        //   } else if (this.searchs === 'Status') {
        //     const StatusMap = [' ', '审核中', '审核通过', '审核未通过']
        //     result = StatusMap.indexOf(this.searchValue)
        //     if (result === -1) {
        //       this.$message({
        //         message: '请填写审核中，审核通过和审核未通过'
        //       })
        //     }
        //   } else {
        //     result = this.searchValue
        //   }
        //   str = str + `&${this.searchs}=${result}`
        // }
        str= str+ this.mixSearch()
      
      fetch(str, {
        method: 'GET',
        headers: {
          Authorization: token,
          'Content-Type': 'application/json'
        }
        }).then(res => {
          if(res.ok){
            return res.json()
          }
          }).then(res => {
            this.list = res.experts,
            this.list.map(data => {
              data.gender = (data.gender > 1 ? "女" : "男")
              data.status = this.statusMap[data.status]
              // console.log("1:status----"+data.status)
              data.birthday=this.tranlateDate(data.birthday)
            }),
            this.totalPage = res.totalPage,
            console.log("this.totalPage:"+this.totalPage)
            this.listLoading = false
            // console.log("2:birth"+this.list.birthday)            
        })
    },
    tranlateDate(birthday) {
      // var before = parseInt(birthday);
      // if(birthday.length === 10)
      // {
      //   before = before*1000;
      // }
      // var dated=new Date(before);
      var dated= new Date(parseInt(birthday) * 1000);
      var Y = dated.getFullYear() + "-";
      var M =
        (dated.getMonth() + 1 < 10
          ? "0" + (dated.getMonth() + 1)
          : dated.getMonth() + 1) + "-";
      var D = dated.getDate() + " ";
      return Y + M + D;
    },
    deletedExperts(){
            this.$router.push({path:'/deletedExpert'})

      // this.page=1
      // this.deleteButton=true
      // this.checkingButton=false
      // this.deletecheckingButton=false
      // this.handleCurrentChange(this.page)
    },
    checkingExperts(){
      this.$router.push({path:'/checkExpert'})
      // this.page=1
      // this.checkingButton=true
      // this.deleteButton=false
      // this.deletecheckingButton=false
      // this.handleCurrentChange(this.page)
    },
    deletecheckingExperts(){
            this.$router.push({path:'/deleteExpert'})

      // this.page=1
      // this.checkingButton=false
      // this.deleteButton=false
      // this.deletecheckingButton=true
      // this.handleCurrentChange(this.page)
    },
    toHome()
    {
      this.$router.push({path:'/manage'})
    },
    mixSearch()
    {
      let str=''
      if (this.upAge !== '' && this.lowAge !== '') {
          str = str + `&Age=${this.lowAge}-${this.upAge}`
        }
        if (this.searchValue !== '' && this.searchs !== '') {
          let result
          if (this.searchs === 'Gender') {
            this.searchValue === '男' ? (result = 1) : (result = 2)
          } else if (this.searchs === 'Status') {
            const StatusMap = [' ', '审核中', '审核通过', '审核未通过']
            result = StatusMap.indexOf(this.searchValue)
            if (result === -1) {
              this.$message({
                message: '请填写审核中，审核通过和审核未通过'
              })
            }
          } else {
            result = this.searchValue
          }
          str = str + `&${this.searchs}=${result}`
        }
      return str
    },
    search()
    {
     this.$router.push({path:`/search`})
      //  this.$router.push({ path: `/new` })
     }
  }
}
</script>

<style lang="scss">
.page{
  text-align: center;
  margin-top: 20px;
  height: 30px;
}
.app-container{
  width: 95%;
  margin: 20px auto;
  border-radius: 5px;
  // box-shadow: 0 0 30px rgba(0,0,0,.1);
  background-color: white
}
.button-add{
  margin-bottom: 20px;
}
.select{
  float: right;
}
.search{
  width: 200px;
  margin-left: 15px;
}
.buttons{
  display: inline-block;
  width: 100%;
}
.searchOption1{
    margin: 0px 5px 8px 0px;
}
.highsearch{
  float: right;
}
.fourbutton
{
  width: 100%;
}
</style>
