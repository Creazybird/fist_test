<template>
 <div>
   <el-button @click="firstclick()">点我啊</el-button>
 </div>
</template>

<script>
export default {
  name: 'HelloWorld',
  data () {
    return {
      msg: 'Welcome to Your Vue.js App'
    }
  },
  methods: {
    firstclick(){
        fetch(`/api/gets`, {
            method: 'GET',
            headers: {
              'Content-Type': 'application/json'
            }
          }).then(res => {
            if (res.ok) {
              return res.json()
            }
          }).then(res=>{
            console.log(res)
          })
    }
  }
}
</script>

<!-- Add "scoped" attribute to limit CSS to this component only -->
<style scoped>
h1, h2 {
  font-weight: normal;
}
ul {
  list-style-type: none;
  padding: 0;
}
li {
  display: inline-block;
  margin: 0 10px;
}
a {
  color: #42b983;
}
</style>
