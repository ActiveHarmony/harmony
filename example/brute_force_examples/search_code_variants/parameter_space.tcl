# param_1, ..., param_n represent n parameters (for e.g., tiling factor, unroll factor, etc.)

harmonyApp OfflineT {
    { harmonyBundle param_1 {
        int {1 100 1} 
    }   
    }
    { harmonyBundle param_2 {
        int {1 100 1} 
    }   
    }
    { harmonyBundle param_3 {
        int {1 100 1}
    }   
    }
    { harmonyBundle param_4 {
        int {1 100 1} 
    }   
    }
    { harmonyBundle param_5 {
        int {1 100 1}
    }   
    }
    { harmonyBundle param_6 {
        int {1 100 1}
    }   
    }
    { obsGoodness -11400 5000 }
    { predGoodness -11400 5000}
} 
 
