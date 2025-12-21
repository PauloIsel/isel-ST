function B = erlangB(A, C)
    B = zeros(size(A));
    for k = 1:length(A)
        a = A(k);
        b = 1;
        for i = 1:C
            b = (a * b) / (i + a * b);
        end
        B(k) = b;
    end
end