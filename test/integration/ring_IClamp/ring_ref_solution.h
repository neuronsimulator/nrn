
struct reference{
    reference(const std::string &test_path):path(test_path){
          ref.reserve(256);
          load();          
    }

    void load(){
         std::string file_name("out.dat.ref"); 
         path += file_name;
std::cout << path << std::endl;
         FILE *fp;
         if ((fp=fopen(path.c_str(), "r")) == 0)
         {
           std::cout << "file was not open" << std::endl;
           return;
         }  
         double dVal;
         int iVal;
         while (!feof(fp))
         {
           fscanf(fp, "%lf\t %d\n", &dVal, &iVal);
           ref.push_back(std::make_pair(dVal, iVal));
         }
         fclose(fp);
    }

    std::size_t size(){
        return ref.size();
    }

    std::pair<double,int> operator[](int i) const{
        return ref[i];
    }

    std::vector<std::pair<double, int> > ref;
    std::string path;
};
